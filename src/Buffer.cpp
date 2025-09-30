/**
 * @file Buffer.cpp
 * @brief Implementation of Buffer - dynamic byte buffer for data storage
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #include "Buffer.h"
 #include <cstring>
 #include <iostream>
 #include <iomanip>
 
 /**
  * @brief Default constructor - creates empty buffer
  * 
  * Initializes buffer with empty state and clear flag set to false.
  * The underlying vector is empty and ready for data operations.
  */
 Buffer::Buffer() : mClear(false) {
     // Empty buffer initialization
 }
 
 /**
  * @brief Copy constructor - creates buffer from another buffer
  * @param other Source buffer to copy from
  * 
  * Creates a new buffer by copying all data from the source buffer.
  * Uses setTo() method for proper deep copy of vector data.
  */
 Buffer::Buffer(const Buffer& other) : mClear(false) {
     setTo(other);
 }
 
 /**
  * @brief Destructor - cleans up buffer resources
  * 
  * Calls clear() to release all allocated memory and reset state.
  * The vector destructor will handle the actual memory deallocation.
  */
 Buffer::~Buffer() {
     clear();
 }
 
 /**
  * @brief Assignment operator - assigns data from another buffer
  * @param other Source buffer to assign from
  * @return Reference to this buffer for chaining
  * 
  * Performs deep copy assignment with self-assignment protection.
  * Uses setTo() method for consistent copying behavior.
  */
 Buffer& Buffer::operator=(const Buffer& other) {
     if (this != &other) {
         setTo(other);
     }
     return *this;
 }
 
 // ========== SIZE AND INITIALIZATION METHODS ==========
 
 /**
  * @brief Set buffer size and initialize with zeros
  * @param len New buffer size in bytes
  * 
  * Resizes the buffer to the specified length and fills all bytes with zeros.
  * This provides a clean initialized buffer for data operations.
  * 
  * @note Previous buffer contents are lost
  * @note Negative length is treated as zero
  */
 void Buffer::setSize(int32_t len) {
     if (len < 0) len = 0;  // Safety check
     mBuf.assign(len, 0);   // Resize and fill with zeros
 }
 
 // ========== DATA COPYING METHODS ==========
 
 /**
  * @brief Copy data from another buffer
  * @param buffer Source buffer to copy from
  * 
  * Performs deep copy of all data from source buffer.
  * This replaces current buffer contents entirely.
  */
 void Buffer::setTo(const Buffer& buffer) {
     mBuf = buffer.mBuf;
 }
 
 /**
  * @brief Copy data from byte array
  * @param buf Source byte array pointer
  * @param len Length of data to copy in bytes
  * 
  * Copies specified number of bytes from source array to buffer.
  * Replaces current buffer contents with new data.
  * 
  * @note Null pointer or zero/negative length results in empty buffer
  */
 void Buffer::setTo(uint8_t* buf, int32_t len) {
     if (buf && len > 0) {
         mBuf.assign(buf, buf + len);
     } else {
         mBuf.clear();  // Clear buffer if invalid input
     }
 }
 
 /**
  * @brief Copy data from char array
  * @param buf Source char array pointer
  * @param len Length of data to copy in bytes
  * 
  * Convenience method that casts char pointer to uint8_t and
  * delegates to the byte array version of setTo().
  */
 void Buffer::setTo(char* buf, int32_t len) {
     setTo(reinterpret_cast<uint8_t*>(buf), len);
 }
 
 /**
  * @brief Append data to existing buffer contents
  * @param buf Source byte array to append
  * @param len Length of data to append in bytes
  * 
  * Adds new data to the end of existing buffer contents.
  * Buffer size increases by the length of appended data.
  * 
  * @note Null pointer or zero/negative length is ignored (no-op)
  */
 void Buffer::append(uint8_t* buf, int32_t len) {
     if (buf && len > 0) {
         mBuf.insert(mBuf.end(), buf, buf + len);
     }
 }
 
 // ========== DATA ACCESS METHODS ==========
 
 /**
  * @brief Get pointer to buffer data
  * @return Pointer to internal buffer data, or nullptr if empty
  * 
  * Provides direct access to internal buffer data for reading or writing.
  * The pointer becomes invalid after buffer resize or destruction.
  * 
  * @warning Pointer validity tied to buffer lifetime and size changes
  * @note Returns nullptr for empty buffers to avoid undefined behavior
  */
 uint8_t* Buffer::data() {
     return mBuf.empty() ? nullptr : mBuf.data();
 }
 
 /**
  * @brief Get current buffer size
  * @return Number of bytes currently stored in buffer
  * 
  * Returns the actual number of bytes of data in the buffer.
  * This may be different from the vector's capacity.
  */
 uint32_t Buffer::size() const {
     return static_cast<uint32_t>(mBuf.size());
 }
 
 /**
  * @brief Check if buffer contains no data
  * @return true if buffer is empty (size == 0)
  * 
  * Efficient way to check if buffer contains any data without
  * getting the actual size value.
  */
 bool Buffer::empty() {
     return mBuf.empty();
 }
 
 // ========== BUFFER MANAGEMENT METHODS ==========
 
 /**
  * @brief Clear all buffer contents and mark as cleared
  * 
  * Removes all data from buffer and sets the clear flag.
  * This frees all allocated memory and resets to empty state.
  * 
  * @note Sets mClear flag to true for debugging purposes
  */
 void Buffer::clear() {
     mBuf.clear();
     mClear = true;
 }
 
 // ========== DEBUG AND UTILITY METHODS ==========
 
 /**
  * @brief Dump current buffer contents to stdout
  * 
  * Prints buffer contents in hexadecimal format for debugging.
  * Shows each byte as a two-digit hex value separated by spaces.
  * Does nothing if buffer is empty.
  */
 void Buffer::dump() {
     if (!mBuf.empty()) {
         dump(mBuf.data(), static_cast<int32_t>(mBuf.size()));
     } else {
         std::cout << "Buffer dump: (empty buffer)" << std::endl;
     }
 }
 
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
  */
 void Buffer::dump(uint8_t* s, int32_t len) {
     if (!s || len <= 0) {
         std::cout << "Buffer dump: (null or empty data)" << std::endl;
         return;
     }
     
     std::cout << "Buffer dump (" << len << " bytes): ";
     
     // Save current format flags
     auto flags = std::cout.flags();
     
     // Set hex format with uppercase and proper width
     std::cout << std::hex << std::uppercase << std::setfill('0');
     
     for (int32_t i = 0; i < len; ++i) {
         std::cout << std::setw(2) << static_cast<unsigned int>(s[i]);
         if (i < len - 1) std::cout << " ";  // Space between bytes
     }
     
     // Restore original format flags
     std::cout.flags(flags);
     std::cout << std::endl;
 }
 
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
  */
 void Buffer::_assign(uint32_t size) {
     mBuf.resize(size);
 }
 
 /**
  * @brief Internal method to release all buffer resources
  * 
  * Equivalent to clear() method. Provided for internal consistency
  * and potential future extensions of cleanup behavior.
  * 
  * @note Currently identical to clear() - may diverge in future versions
  */
 void Buffer::_release() {
     clear();
 }