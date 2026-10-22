#pragma once

#include <algorithm>
#include <deque>
#include <string>
#include <string_view>

namespace Randomizer {

// Independent of the renderer so line limits use the same rules in menus,
// races, and tests. The logger controls when the stored lines are visible.
class ArchipelagoLogBuffer {
public:
    static constexpr size_t maxLines = 5;
    static constexpr size_t lineColumns = 76;
    // DrawUIText (00492150) has a 256-byte local text area. Reserve room for
    // the native UTF-8 ellipsis (three bytes) and the terminating null byte.
    static constexpr size_t nativeTextBufferBytes = 256;
    static constexpr size_t utf8EllipsisBytes = 3;
    static constexpr size_t nullTerminatorBytes = 1;
    static constexpr size_t maxLineBytes = nativeTextBufferBytes - utf8EllipsisBytes - nullTerminatorBytes;

    // Returns whether at least one nonblank line was added.
    bool Add(std::string_view message) {
        std::string line;
        size_t columns = 0;
        bool added = false;

        const auto emitLine = [&]() {
            const bool containsText = std::any_of(line.begin(), line.end(), [](char character) {
                return character != ' ';
            });
            if (containsText) {
                lines.push_back(line);
                if (lines.size() > maxLines) {
                    lines.pop_front();
                }
                added = true;
            }
            line.clear();
            columns = 0;
        };

        for (size_t offset = 0; offset < message.size();) {
            const unsigned char character = static_cast<unsigned char>(message[offset]);
            if (character == '\n' || character == '\r') {
                emitLine();
                ++offset;
                continue;
            }

            // Include the first byte and all continuation bytes of this UTF-8
            // character, so wrapping never separates them. This assumes valid
            // UTF-8 input; it does not validate the encoding.
            size_t bytes = 1;
            while (offset + bytes < message.size() &&
                   IsUtf8ContinuationByte(static_cast<unsigned char>(message[offset + bytes]))) {
                ++bytes;
            }

            // DrawUIText copies into a 256-byte native buffer. Leave space for
            // its ellipsis and never split a UTF-8 character between lines.
            if (columns == lineColumns || line.size() + bytes > maxLineBytes) {
                const size_t space = line.find_last_of(' ');
                if (space != std::string::npos && space > 0) {
                    std::string remainder = line.substr(space + 1);
                    line.resize(space);
                    emitLine();
                    line = remainder;
                    for (unsigned char byte : line) {
                        // Each non-continuation byte starts a new character.
                        if (!IsUtf8ContinuationByte(byte)) {
                            ++columns;
                        }
                    }
                } else {
                    emitLine();
                }
            }

            // ASCII 0x00-0x1f are control characters, including NUL and tab;
            // 0x7f is DEL. Newline and carriage return were handled above.
            const bool isControlCharacter = character < 0x20 || character == 0x7f;
            if (isControlCharacter) {
                line += ' ';
            } else {
                line.append(message.substr(offset, bytes));
            }
            ++columns;
            offset += bytes;
        }
        emitLine();

        return added;
    }

    const std::deque<std::string>& GetLines() const {
        return lines;
    }

private:
    static bool IsUtf8ContinuationByte(unsigned char byte) {
        // UTF-8 continuation bytes have the binary prefix 10xxxxxx, which
        // corresponds to the inclusive byte range 0x80-0xbf.
        return byte >= 0x80 && byte <= 0xbf;
    }

    std::deque<std::string> lines;
};

} // namespace Randomizer
