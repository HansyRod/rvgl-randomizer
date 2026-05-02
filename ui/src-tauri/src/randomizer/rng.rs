use std::time::{SystemTime, UNIX_EPOCH};

// ============================================================================
// Simple LCG-based RNG — no external crate needed
// Uses current system time as seed.
// ============================================================================
pub struct Rng {
    state: u64,
    seed: String,
}

impl Rng {
    pub fn new() -> Self {
        // Seed from system time (nanoseconds)
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.subsec_nanos() as u64 ^ d.as_secs().wrapping_mul(6364136223846793005))
            .unwrap_or(12345);
        let seed_str = format!("{:x}", nanos);
        Rng { state: nanos | 1, seed: seed_str }
    }

    /// Returns the seed string used for this RNG
    pub fn seed(&self) -> &str {
        &self.seed
    }

    /// Returns a random usize in [0, n)
    pub fn next_usize(&mut self, n: usize) -> usize {
        // LCG constants from Knuth
        self.state = self.state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        ((self.state >> 33) as usize) % n
    }

    /// Returns a random i32 in [lo, hi] (inclusive)
    pub fn next_i32(&mut self, lo: i32, hi: i32) -> i32 {
        let range = (hi - lo + 1) as usize;
        lo + self.next_usize(range) as i32
    }

    /// Shuffles a vector in place
    pub fn shuffle<T>(&mut self, data: &mut [T]) {
        for i in (1..data.len()).rev() {
            let j = self.next_usize(i + 1);
            data.swap(i, j);
        }
    }
}
