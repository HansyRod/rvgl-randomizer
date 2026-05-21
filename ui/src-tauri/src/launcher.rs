// ============================================================================
// launcher.rs
//
// Rust port of Launcher.cpp.
// Starts RVGL in a suspended state, injects the mod DLL via the standard
// VirtualAllocEx + WriteProcessMemory + CreateRemoteThread(LoadLibraryA)
// technique, then resumes the game.
// ============================================================================

use serde::Serialize;
use std::path::{Path, PathBuf};
use tauri::Manager;

#[derive(Serialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct LaunchResult {
    pub pid: u32,
}

fn quote_arg(arg: &str) -> String {
    if arg.contains(' ') || arg.contains('\t') {
        format!("\"{}\"", arg)
    } else {
        arg.to_string()
    }
}

fn split_extra_args(args: &str) -> Vec<String> {
    let mut result = Vec::new();
    let mut current = String::new();
    let mut in_quotes = false;

    for ch in args.chars() {
        if ch == '"' {
            in_quotes = !in_quotes;
            continue;
        }

        if !in_quotes && ch.is_whitespace() {
            if !current.is_empty() {
                result.push(std::mem::take(&mut current));
            }
            continue;
        }

        current.push(ch);
    }

    if !current.is_empty() {
        result.push(current);
    }

    result
}

fn resolve_mod_library(app_handle: &tauri::AppHandle) -> Result<PathBuf, String> {
    // --- Resolve randomizer.dll or randomizer.so from the Tauri resource directory ---
    let library_name = if cfg!(target_os = "windows") {
        "randomizer.dll"
    } else {
        "randomizer.so"
    };

    let resource_name = format!("resources/{library_name}");
    let mut library_path = app_handle
        .path()
        .resolve(&resource_name, tauri::path::BaseDirectory::Resource)
        .map_err(|e| format!("Cannot resolve resource: {e}"))?;

    // Fallback for development: if not found in the resolved resource dir,
    // check the source resources folder relative to the manifest directory.
    if !library_path.exists() && cfg!(debug_assertions) {
        let source_path = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("resources")
            .join(library_name);
        if source_path.exists() {
            library_path = source_path;
        }
    }

    if !library_path.exists() {
        return Err(format!(
            "{library_name} not found at: {}\nRun 'npm run build:dll' to build it first.",
            library_path.display()
        ));
    }

    Ok(library_path)
}

fn build_launcher_args(
    rvgl_exe_path: &str,
    extra_args: &str,
    packlist: Option<Vec<String>>,
    profile_name: &str,
) -> Result<Vec<String>, String> {
    // --- Handle Launcher-mode args (basepath, prefpath, packlist) ---
    //
    // Launcher install directory structure:
    //   <basepath>/              ← great-grandparent of rvgl.exe
    //     packs/                 ← grandparent of rvgl.exe; packlist files go here
    //       <platform>/
    //         rvgl.exe
    //     save/                  ← prefpath
    //
    // Argument order: -basepath → -prefpath → -packlist → user extra_args → -profile
    let mut launcher_args: Vec<String> = Vec::new();

    if let Some(packs) = packlist {
        // packs_dir  = grandparent of rvgl.exe  (e.g. C:\Games\RVGL\packs)
        // basepath   = parent of packs_dir       (e.g. C:\Games\RVGL)
        let exe_path = Path::new(rvgl_exe_path);
        let packs_dir = exe_path
            .parent()
            .and_then(|p| p.parent())
            .ok_or("Could not resolve 'packs' directory for Launcher install.")?;

        let basepath = packs_dir
            .parent()
            .ok_or("Could not resolve base path for Launcher install (packs dir has no parent).")?;

        let prefpath = basepath.join("save");

        // Write the packlist file into the packs directory.
        let packlist_filename = format!("rvgl-randomizer-{profile_name}.txt");
        let packlist_path = packs_dir.join(&packlist_filename);
        let quoted_packlist = packs
            .into_iter()
            .map(|pack| format!("\"{}\"", pack))
            .collect::<Vec<_>>()
            .join("\n");
        std::fs::write(&packlist_path, quoted_packlist)
            .map_err(|e| format!("Failed to write packlist {packlist_filename}: {e}"))?;

        launcher_args.push("-basepath".to_string());
        launcher_args.push(basepath.to_string_lossy().into_owned());
        launcher_args.push("-prefpath".to_string());
        launcher_args.push(prefpath.to_string_lossy().into_owned());
        launcher_args.push("-packlist".to_string());
        launcher_args.push(format!("rvgl-randomizer-{profile_name}"));
    }

    // Append any user-supplied extra args after the launcher-specific ones.
    if !extra_args.is_empty() {
        launcher_args.extend(split_extra_args(extra_args));
    }

    // Always append -profile last.
    launcher_args.push("-profile".to_string());
    launcher_args.push(profile_name.to_string());

    Ok(launcher_args)
}

// ============================================================================
// launch_game Tauri command
//
// rvgl_exe_path : absolute path to rvgl.exe (from the UI's installPath)
// extra_args    : optional extra CLI args forwarded to RVGL (e.g. "-window")
// ============================================================================
#[tauri::command]
pub fn launch_game(
    app_handle: tauri::AppHandle,
    rvgl_exe_path: String,
    extra_args: String,
    config_path: String,
    packlist: Option<Vec<String>>,
    profile_name: String,
) -> Result<LaunchResult, String> {
    let mod_library_path = resolve_mod_library(&app_handle)?;
    let launcher_args = build_launcher_args(&rvgl_exe_path, &extra_args, packlist, &profile_name)?;

    platform::launch_game_platform(
        rvgl_exe_path,
        launcher_args,
        config_path,
        mod_library_path,
    )
}

// ============================================================================
// is_process_running Tauri command
//
// Returns true if the process with the given PID is still running.
// Uses OpenProcess + GetExitCodeProcess with PROCESS_QUERY_LIMITED_INFORMATION,
// which is the least-privilege access right needed for this check.
// ============================================================================
#[tauri::command]
pub fn is_process_running(pid: u32) -> bool {
    platform::is_process_running_platform(pid)
}

#[cfg(target_os = "windows")]
mod platform {
    use super::{quote_arg, LaunchResult};
    use std::ffi::CString;
    use std::path::{Path, PathBuf};

    use windows::core::{PCSTR, PSTR};
    use windows::Win32::{
        Foundation::{CloseHandle, STILL_ACTIVE},
        System::{
            Diagnostics::Debug::WriteProcessMemory,
            LibraryLoader::{GetModuleHandleA, GetProcAddress},
            Memory::{
                VirtualAllocEx, VirtualFreeEx, MEM_COMMIT, MEM_RELEASE, MEM_RESERVE,
                PAGE_READWRITE,
            },
            Threading::{
                CreateProcessA, CreateRemoteThread, GetExitCodeProcess, GetExitCodeThread,
                OpenProcess, ResumeThread, TerminateProcess, WaitForSingleObject,
                CREATE_SUSPENDED, INFINITE, LPTHREAD_START_ROUTINE, PROCESS_INFORMATION,
                PROCESS_QUERY_LIMITED_INFORMATION, STARTUPINFOA,
            },
        },
    };

    // ============================================================================
    // inject_dll
    //
    // Allocates a page in the target process, writes the DLL path, then spawns a
    // remote thread that calls LoadLibraryA.  Blocks until the load completes.
    // Returns Err with a message if any step fails.
    // ============================================================================
    unsafe fn inject_dll(
        process: windows::Win32::Foundation::HANDLE,
        dll_path: &str,
    ) -> Result<(), String> {
        let path_cstr =
            CString::new(dll_path).map_err(|e| format!("Invalid DLL path string: {e}"))?;
        let path_bytes = path_cstr.as_bytes_with_nul();

        // Allocate memory in the target process for the DLL path string.
        let remote_mem = VirtualAllocEx(
            process,
            None,
            path_bytes.len(),
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE,
        );
        if remote_mem.is_null() {
            return Err("VirtualAllocEx failed - cannot allocate remote memory.".into());
        }

        // Write the DLL path into the remote process.
        let write_result = WriteProcessMemory(
            process,
            remote_mem,
            path_bytes.as_ptr().cast(),
            path_bytes.len(),
            None,
        );
        if write_result.is_err() {
            let _ = VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
            return Err("WriteProcessMemory failed - cannot write DLL path.".into());
        }

        // Resolve LoadLibraryA from kernel32 in this process.
        // The address is identical in every process on the same OS session.
        let k32 = GetModuleHandleA(PCSTR(b"kernel32.dll\0".as_ptr()))
            .map_err(|e| format!("GetModuleHandleA(kernel32) failed: {e}"))?;

        let load_library_raw = GetProcAddress(k32, PCSTR(b"LoadLibraryA\0".as_ptr()))
            .ok_or("GetProcAddress(LoadLibraryA) failed.")?;

        // Transmute to the thread-start-routine signature.
        // Safe on x64 Windows: same calling convention, pointer-sized return value.
        let thread_fn: LPTHREAD_START_ROUTINE = Some(std::mem::transmute(load_library_raw));

        let remote_thread = CreateRemoteThread(
            process,
            None,
            0,
            thread_fn,
            Some(remote_mem),
            0,
            None,
        )
        .map_err(|e| format!("CreateRemoteThread failed: {e}"))?;

        // Block until LoadLibraryA finishes inside the target process.
        WaitForSingleObject(remote_thread, INFINITE);

        let mut exit_code: u32 = 0;
        let _ = GetExitCodeThread(remote_thread, &mut exit_code);

        CloseHandle(remote_thread).ok();
        let _ = VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);

        // LoadLibraryA returns the HMODULE (non-zero) on success.
        if exit_code == 0 {
            Err("LoadLibraryA returned NULL inside RVGL - DLL failed to load.".into())
        } else {
            Ok(())
        }
    }

    pub fn launch_game_platform(
        rvgl_exe_path: String,
        launcher_args: Vec<String>,
        config_path: String,
        mod_library_path: PathBuf,
    ) -> Result<LaunchResult, String> {
        let dll_path_str = mod_library_path.to_string_lossy().into_owned();

        // --- Build working directory ---
        let working_dir = Path::new(&rvgl_exe_path)
            .parent()
            .map(|p| p.to_string_lossy().into_owned())
            .unwrap_or_else(|| ".".to_string());

        // --- NEW: Set Environment Variable ---
        // Child processes spawned by CreateProcessA inherit the parent's environment.
        std::env::set_var("RVGL_RANDOMIZER_CONFIG", &config_path);

        let final_args = launcher_args
            .iter()
            .map(|arg| quote_arg(arg))
            .collect::<Vec<_>>()
            .join(" ");

        // --- Build CreateProcessA arguments ---
        let cmd_line = if final_args.is_empty() {
            format!("\"{}\"", rvgl_exe_path)
        } else {
            format!("\"{}\" {}", rvgl_exe_path, final_args)
        };

        let exe_cstr =
            CString::new(rvgl_exe_path.as_str()).map_err(|e| format!("Bad exe path: {e}"))?;
        let mut cmd_bytes = cmd_line.into_bytes();
        cmd_bytes.push(0); // null-terminate for PSTR
        let work_cstr =
            CString::new(working_dir.as_str()).map_err(|e| format!("Bad working dir: {e}"))?;

        unsafe {
            let mut si = STARTUPINFOA::default();
            si.cb = std::mem::size_of::<STARTUPINFOA>() as u32;
            let mut pi = PROCESS_INFORMATION::default();

            // Start RVGL suspended so we can inject before any game code runs.
            CreateProcessA(
                PCSTR(exe_cstr.as_ptr().cast()),
                Some(PSTR(cmd_bytes.as_mut_ptr())),
                None,
                None,
                false,
                CREATE_SUSPENDED,
                None,
                PCSTR(work_cstr.as_ptr().cast()),
                &si,
                &mut pi,
            )
            .map_err(|e| format!("CreateProcess failed: {e}"))?;

            // Inject the DLL while RVGL's main thread is still suspended.
            if let Err(msg) = inject_dll(pi.hProcess, &dll_path_str) {
                let _ = TerminateProcess(pi.hProcess, 1);
                let _ = CloseHandle(pi.hThread);
                let _ = CloseHandle(pi.hProcess);
                return Err(format!("DLL injection failed: {msg}"));
            }

            // Hooks are installed — let RVGL run.
            ResumeThread(pi.hThread);
            let pid = pi.dwProcessId;

            let _ = CloseHandle(pi.hThread);
            let _ = CloseHandle(pi.hProcess);

            Ok(LaunchResult { pid })
        }
    }

    pub fn is_process_running_platform(pid: u32) -> bool {
        unsafe {
            let handle = match OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid) {
                Ok(h) => h,
                Err(_) => return false,
            };

            let mut exit_code: u32 = 0;
            let still_running = GetExitCodeProcess(handle, &mut exit_code)
                .is_ok()
                && exit_code == STILL_ACTIVE.0 as u32;

            let _ = CloseHandle(handle);
            still_running
        }
    }
}

#[cfg(target_os = "linux")]
mod platform {
    use super::LaunchResult;
    use std::path::{Path, PathBuf};
    use std::process::Command;

    pub fn launch_game_platform(
        rvgl_exe_path: String,
        launcher_args: Vec<String>,
        config_path: String,
        mod_library_path: PathBuf,
    ) -> Result<LaunchResult, String> {
        let working_dir = Path::new(&rvgl_exe_path)
            .parent()
            .map(Path::to_path_buf)
            .unwrap_or_else(|| PathBuf::from("."));

        let mut command = Command::new(&rvgl_exe_path);
        command
            .current_dir(working_dir)
            .args(launcher_args)
            .env("LD_PRELOAD", mod_library_path)
            .env("RVGL_RANDOMIZER_CONFIG", config_path);

        let child = command
            .spawn()
            .map_err(|e| format!("Failed to launch RVGL with LD_PRELOAD: {e}"))?;

        Ok(LaunchResult { pid: child.id() })
    }

    pub fn is_process_running_platform(pid: u32) -> bool {
        Path::new("/proc").join(pid.to_string()).exists()
    }
}

#[cfg(not(any(target_os = "windows", target_os = "linux")))]
mod platform {
    use super::LaunchResult;
    use std::path::PathBuf;

    pub fn launch_game_platform(
        _rvgl_exe_path: String,
        _launcher_args: Vec<String>,
        _config_path: String,
        _mod_library_path: PathBuf,
    ) -> Result<LaunchResult, String> {
        Err("RVGL launching is only implemented for Windows and Linux.".to_string())
    }

    pub fn is_process_running_platform(_pid: u32) -> bool {
        false
    }
}
