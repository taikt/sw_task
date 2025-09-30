/**
 * @file Buffer.h
 * @brief Buffer class for dynamic byte storage and manipulation
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include <vector>
 #include <cstdint>
 #include <memory>
 
 namespace swt {
 /**
  * @class Buffer
  * @brief Dynamic byte buffer for data storage and manipulation
  * 
  * Provides a dynamic byte array with various utility methods for
  * data manipulation, copying, and debugging output. Built on top
  * of std::vector<uint8_t> for efficient memory management.
  * 
  * Key features:
  * - **Dynamic sizing**: Automatic memory management with resize operations
  * - **Data copying**: Multiple methods for copying from various sources
  * - **Append operations**: Efficient data concatenation
  * - **Debug support**: Hex dump utilities for debugging
  * - **Safe access**: Null pointer protection and bounds checking
  * - **STL integration**: Compatible with standard library algorithms
  * 
  * @code{.cpp}
  * // Create and initialize buffer
  * Buffer buffer;
  * buffer.setSize(1024);  // 1KB buffer initialized with zeros
  * 
  * // Copy data from array
  * uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  * buffer.setTo(data, sizeof(data));
  * 
  * // Append more data
  * uint8_t more[] = {0x05, 0x06};
  * buffer.append(more, sizeof(more));
  * 
  * // Debug output
  * buffer.dump();  // Prints: "Buffer dump (6 bytes): 01 02 03 04 05 06"
  * 
  * // Access data
  * uint8_t* ptr = buffer.data();
  * uint32_t size = buffer.size();
  * @endcode
  * 
  * @note Thread-safe for read-only operations when not being modified
  * @note Not thread-safe for concurrent modifications
  * @warning Pointer returned by data() becomes invalid after resize operations
  * 
  * @see \ref swt::Buffer::setSize "setSize()", \ref swt::Buffer::setTo "setTo()", \ref swt::Buffer::append "append()", \ref swt::Buffer::data "data()", \ref swt::Buffer::size "size()", \ref swt::Buffer::clear "clear()", \ref swt::Buffer::dump "dump()"
  */
 class Buffer {
 public:
     // ========== CONSTRUCTORS AND DESTRUCTOR ==========
 
     /**
      * @brief Default constructor - creates empty buffer
      * 
      * Initializes buffer with empty state and clear flag set to false.
      * The underlying vector is empty and ready for data operations.
      * 
      * @note No memory allocated until first data operation
      */
     Buffer();
 
     /**
      * @brief Copy constructor - creates buffer from another buffer
      * @param other Source buffer to copy from
      * 
      * Creates a new buffer by performing deep copy of all data from 
      * the source buffer. Both buffers are independent after construction.
      * 
      * @note Performs deep copy - changes to either buffer don't affect the other
      */
     Buffer(const Buffer& other);
 
     /**
      * @brief Destructor - cleans up buffer resources
      * 
      * Automatically releases all allocated memory. The underlying
      * std::vector destructor handles the actual memory deallocation.
      */
     ~Buffer();
 
     /**
      * @brief Assignment operator - assigns data from another buffer
      * @param other Source buffer to assign from
      * @return Reference to this buffer for chaining
      * 
      * Performs deep copy assignment with self-assignment protection.
      * Replaces current buffer contents with source buffer data.
      * 
      * @code{.cpp}
      * Buffer buf1, buf2;
      * buf1.setTo(data, size);
      * buf2 = buf1;  // Deep copy assignment
      * @endcode
      * 
      * @note Self-assignment safe
      * @note Previous buffer contents are lost
      * @see \ref swt::Buffer::setTo "setTo()"
      */
     Buffer& operator=(const Buffer& other);
 
     // ========== SIZE AND INITIALIZATION METHODS ==========
 
     /**
      * @brief Set buffer size and initialize with zeros
      * @param len New buffer size in bytes
      * 
      * Resizes the buffer to the specified length and fills all bytes with zeros.
      * This provides a clean initialized buffer for data operations.
      * 
      * @code{.cpp}
      * Buffer buffer;
      * buffer.setSize(1024);  // 1KB buffer filled with zeros
      * @endcode
      * 
      * @note Previous buffer contents are lost
      * @note Negative length is treated as zero (creates empty buffer)
      * @note More efficient than resize + manual zero-fill
      * @see \ref swt::Buffer::clear "clear()"
      */
     void setSize(int32_t len);
 
     // ========== DATA COPYING METHODS ==========
 
     /**
      * @brief Copy data from another buffer
      * @param buffer Source buffer to copy from
      * 
      * Performs deep copy of all data from source buffer.
      * This replaces current buffer contents entirely.
      * 
      * @code{.cpp}
      * Buffer source, dest;
      * source.setTo(data, size);
      * dest.setTo(source);  // dest now contains copy of source data
      * @endcode
      * 
      * @note Deep copy - source and destination are independent
      * @note Previous destination contents are lost
      * @see \ref swt::Buffer::setTo "setTo()"
      */
     void setTo(const Buffer& buffer);
 
     /**
      * @brief Copy data from byte array
      * @param buf Source byte array pointer
      * @param len Length of data to copy in bytes
      * 
      * Copies specified number of bytes from source array to buffer.
      * Replaces current buffer contents with new data.
      * 
      * @code{.cpp}
      * uint8_t data[] = {0x01, 0x02, 0x03};
      * buffer.setTo(data, sizeof(data));
      * @endcode
      * 
      * @note Null pointer or zero/negative length results in empty buffer
      * @note Previous buffer contents are lost
      * @warning Source pointer must remain valid during copy operation
      * @see \ref swt::Buffer::setTo "setTo()"
      */
     void setTo(uint8_t* buf, int32_t len);
 
     /**
      * @brief Copy data from char array
      * @param buf Source char array pointer
      * @param len Length of data to copy in bytes
      * 
      * Convenience method that casts char pointer to uint8_t and
      * delegates to the byte array version of setTo().
      * 
      * @code{.cpp}
      * char str[] = "Hello";
      * buffer.setTo(str, strlen(str));
      * @endcode
      * 
      * @note Same behavior as uint8_t* version
      * @note Useful for string data without null terminator
      * @see \ref swt::Buffer::setTo "setTo()"
      */
     void setTo(char* buf, int32_t len);
 
     /**
      * @brief Append data to existing buffer contents
      * @param buf Source byte array to append
      * @param len Length of data to append in bytes
      * 
      * Adds new data to the end of existing buffer contents.
      * Buffer size increases by the length of appended data.
      * 
      * @code{.cpp}
      * buffer.setTo(data1, len1);     // Initial data
      * buffer.append(data2, len2);    // Append more data
      * // Buffer now contains data1 + data2
      * @endcode
      * 
      * @note Null pointer or zero/negative length is ignored (no-op)
      * @note Efficient - uses vector::insert for optimal performance
      * @note Previous buffer contents are preserved
      * @see \ref swt::Buffer::append "append()"
      */
     void append(uint8_t* buf, int32_t len);
 
     // ========== DATA ACCESS METHODS ==========
 
     /**
      * @brief Get pointer to buffer data
      * @return Pointer to internal buffer data, or nullptr if empty
      * 
      * Provides direct access to internal buffer data for reading or writing.
      * The pointer becomes invalid after buffer resize or destruction.
      * 
      * @code{.cpp}
      * uint8_t* ptr = buffer.data();
      * if (ptr) {
      *     // Safe to access ptr[0] through ptr[buffer.size()-1]
      *     memcpy(destination, ptr, buffer.size());
      * }
      * @endcode
      * 
      * @warning Pointer validity tied to buffer lifetime and size changes
      * @warning Do not access beyond buffer.size() bytes
      * @note Returns nullptr for empty buffers to avoid undefined behavior
      * @note Pointer may change after resize, append, or setTo operations
      * @see \ref swt::Buffer::size "size()"
      */
     uint8_t* data();
 
     /**
      * @brief Get current buffer size
      * @return Number of bytes currently stored in buffer
      * 
      * Returns the actual number of bytes of data in the buffer.
      * This represents the valid data size, not the allocated capacity.
      * 
      * @code{.cpp}
      * for (uint32_t i = 0; i < buffer.size(); ++i) {
      *     process(buffer.data()[i]);
      * }
      * @endcode
      * 
      * @note This may be different from the vector's capacity
      * @note Constant time operation - O(1)
      * @see \ref swt::Buffer::data "data()"
      */
     uint32_t size() const;
 
     /**
      * @brief Check if buffer contains no data
      * @return true if buffer is empty (size == 0)
      * 
      * Efficient way to check if buffer contains any data without
      * getting the actual size value. Preferred over size() == 0.
      * 
      * @code{.cpp}
      * if (!buffer.empty()) {
      *     // Process buffer data
      *     processData(buffer.data(), buffer.size());
      * }
      * @endcode
      * 
      * @note More efficient than size() == 0 check
      * @note Constant time operation - O(1)
      * @see \ref swt::Buffer::size "size()"
      */
     bool empty();
 
     // ========== BUFFER MANAGEMENT METHODS ==========
 
     /**
      * @brief Clear all buffer contents and mark as cleared
      * 
      * Removes all data from buffer and sets the clear flag.
      * This frees all allocated memory and resets to empty state.
      * 
      * @code{.cpp}
      * buffer.clear();
      * assert(buffer.empty());
      * assert(buffer.size() == 0);
      * @endcode
      * 
      * @note Sets internal mClear flag to true for debugging purposes
      * @note Frees allocated memory (not just marks as empty)
      * @note After clear(), data() returns nullptr
      * @see \ref swt::Buffer::empty "empty()", \ref swt::Buffer::size "size()"
      */
     void clear();
 
     // ========== DEBUG AND UTILITY METHODS ==========
 
     /**
      * @brief Dump current buffer contents to stdout
      * 
      * Prints buffer contents in hexadecimal format for debugging.
      * Shows each byte as a two-digit hex value separated by spaces.
      * Does nothing if buffer is empty.
      * 
      * @code{.cpp}
      * uint8_t data[] = {0x01, 0x02, 0x03};
      * buffer.setTo(data, 3);
      * buffer.dump();  // Output: "Buffer dump (3 bytes): 01 02 03"
      * @endcode
      * 
      * @note Non-destructive operation - buffer contents unchanged
      * @note Uses uppercase hexadecimal format
      * @note Outputs to std::cout with automatic newline
      * @see \ref swt::Buffer::dump "dump()"
      */
     void dump();
 
     /**
      * @brief Static method to dump byte array contents to stdout
      * @param s Pointer to byte array to dump
      * @param len Number of bytes to dump
      * 
      * Utility method for dumping arbitrary byte arrays in hex format.
      * Useful for debugging data that's not in a Buffer object.
      * 
      * Format: "Buffer dump (N bytes): XX XX XX ..."
      * where XX represents each byte in uppercase hexadecimal.
      * 
      * @code{.cpp}
      * uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
      * Buffer::dump(data, 4);  // Output: "Buffer dump (4 bytes): DE AD BE EF"
      * @endcode
      * 
      * @note Static method - can be called without Buffer instance
      * @note Handles null pointer and invalid length gracefully
      * @note Same format as instance dump() method
      * @see \ref swt::Buffer::dump "dump()"
      */
     static void dump(uint8_t* s, int32_t len);
 
 private:
     // ========== INTERNAL METHODS ==========
 
     /**
      * @brief Internal method to resize buffer without initialization
      * @param size New buffer size in bytes
      * 
      * Low-level resize operation that changes buffer size without
      * initializing new elements. Used internally for performance.
      * 
      * @note For internal use - prefer setSize() for external calls
      * @warning New elements may contain garbage data
      * @note More efficient than setSize() when initialization not needed
      */
     void _assign(uint32_t size);
 
     /**
      * @brief Internal method to release all buffer resources
      * 
      * Equivalent to clear() method. Provided for internal consistency
      * and potential future extensions of cleanup behavior.
      * 
      * @note Currently identical to clear() - may diverge in future versions
      * @note For internal use - prefer clear() for external calls
      * @see \ref swt::Buffer::clear "clear()"
      */
     void _release();
 
 private:
     // ========== MEMBER VARIABLES ==========
 
     /**
      * @brief Internal byte storage container
      * 
      * Uses std::vector<uint8_t> for efficient dynamic memory management.
      * Provides automatic memory allocation, deallocation, and reallocation.
      */
     std::vector<uint8_t> mBuf;
 
     /**
      * @brief Clear flag for debugging purposes
      * 
      * Set to true when clear() is called, can be used for debugging
      * to track buffer lifecycle and detect use-after-clear scenarios.
      */
     bool mClear;
 };
 }