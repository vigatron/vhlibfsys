/* ======================================================================================
 * Library       : vhlibfsys
 * Description   : A small C++17 filesystem helper library with 64-bit binary file I/O.
 * Revision      : 0.1.1
 * Source        : https://github.com/vigatron/vhlibfsys
 * Disclaimer    : Provided "AS IS", without warranty.
 * License       : MIT
 * File          : src/vhfile.hpp
 * Content size  : 1085
 * Date / Time   : 27-10-2026 22:12:08
 * MD5           : bf357cf7245dc4e460d8f8273d9caabe
 * Notes         : MD5 = file content without header/footer
 * Encoding      : UTF-8
 * Author        : Viktor Glebov / V01G04A81
 * Copyright     : © 2026 Viktor Glebov
 * ========================[ BEGIN FILE CONTENT ]====================================== */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

/**
 * @brief RAII wrapper around a C FILE* for binary file I/O with 64-bit offsets/sizes.
 *
 * @details
 * VHFile provides move-only ownership of a FILE* handle opened in binary mode.
 * The file is opened on demand: read-type operations open with "rb", write operations
 * open with "wb". Supports 64-bit file positions/sizes on Windows and POSIX.
 */
class VHFile
{
public:
    /**
     * @brief Default constructor. Creates a closed file object with empty filename.
     */
    VHFile();

    /**
     * @brief Constructor that stores filename. Does not open the file.
     * @param filename Path to the file to be opened later.
     */
    explicit VHFile(const std::string &filename);

    /**
     * @brief Destructor. Closes the file if open.
     */
    ~VHFile();

    /**
     * @brief Copy constructor is deleted (file handle must have unique ownership).
     */
    VHFile(const VHFile &) = delete;

    /**
     * @brief Copy assignment operator is deleted (file handle must have unique ownership).
     */
    VHFile &operator=(const VHFile &) = delete;

    /**
     * @brief Move constructor. Transfers ownership of file handle and filename.
     * @param other Source object to move from. Left in closed state.
     */
    VHFile(VHFile &&other) noexcept;

    /**
     * @brief Move assignment operator. Closes current handle and transfers ownership from other.
     * @param other Source object to move from. Left in closed state.
     * @return Reference to this object.
     */
    VHFile &operator=(VHFile &&other) noexcept;

    /**
     * @brief Assigns a new filename. Closes current file if open, does not open new file.
     * @param filename New path to file.
     */
    void assign(const std::string &filename);

    /**
     * @brief Gets file size in bytes.
     * @details If file is closed, opens it read-only. If file is open for writing, flushes before stat.
     * @param size Pointer to receive size. Must not be nullptr.
     * @return true on success, false on failure.
     */
    bool filesize(std::uint64_t *size);

    /**
     * @brief Gets current file position.
     * @details If file is closed, opens it read-only.
     * @param pos Pointer to receive position. Must not be nullptr.
     * @return true on success, false on failure.
     */
    bool pos(std::uint64_t *pos);

    /**
     * @brief Seeks to absolute position.
     * @details If file is closed, opens it read-only. Always uses SEEK_SET.
     * @param pos Absolute byte offset from beginning of file.
     * @return true on success, false on failure.
     */
    bool seek(std::uint64_t pos);

    /**
     * @brief Reads exactly sz bytes into ptr.
     * @details If file is closed, opens read-only. Returns false if file is open for writing.
     *          Loops until all bytes read or error. If seekoffs != 0, seeks to seekoffs first.
     * @param ptr Destination buffer. Must not be nullptr if sz != 0.
     * @param sz Number of bytes to read.
     * @param seekoffs Absolute offset to seek to before reading, or 0 for no seek.
     * @return true if exactly sz bytes read (or sz == 0), false on error/short read.
     */
    bool read(void *ptr, std::size_t sz, std::uint64_t seekoffs = 0);

    /**
     * @brief Writes exactly sz bytes from ptr.
     * @details If file is closed, creates/truncates for writing. Returns false if file is open for reading.
     *          Loops until all bytes written or error.
     * @param ptr Source buffer. Must not be nullptr if sz != 0.
     * @param sz Number of bytes to write.
     * @return true if exactly sz bytes written (or sz == 0), false on error/short write.
     */
    bool write(const void *ptr, std::size_t sz);

    /**
     * @brief Opens file for reading in binary mode ("rb").
     * @details Idempotent if already open read-only. Returns false if already open for writing.
     * @return true if open for reading, false on failure.
     */
    bool openfile();

    /**
     * @brief Opens file for writing in binary mode ("wb"), creating/truncating.
     * @details Idempotent if already open writable. Returns false if already open for reading.
     * @return true if open for writing, false on failure.
     */
    bool createfile();

    /**
     * @brief Closes file handle.
     * @details Resets handle to nullptr even if fclose fails. Safe to call multiple times.
     * @return true if fclose succeeded, false if already closed or fclose failed.
     */
    bool closefile();

    /**
     * @brief Checks if file is open.
     * @return true if file handle != nullptr.
     */
    bool isopen() const;

    /**
     * @brief Checks if file is closed.
     * @return true if file handle == nullptr.
     */
    bool isclosed() const;

private:
    /**
     * @brief Prints read error message to stderr with filename and errno.
     */
    void dmperr();

    /**
     * @brief Prints write/create error message to stderr with filename and errno.
     */
    void dmpwerr();

    std::string fname;  ///< Filename associated with object
    FILE *file;         ///< Open FILE* handle, nullptr if closed
    bool writable;      ///< true if opened by createfile() for writing
};
/* ========================[  END FILE CONTENT  ]========================
 * Library          : vhlibfsys
 * File             : src/vhfile.hpp
 * Revision         : 0.1.1
 * Content size     : 1085
 * Date / Time      : 27-10-2026 22:12:08
 * MD5              : bf357cf7245dc4e460d8f8273d9caabe
 * Copyright        : © 2026 Viktor Glebov
 * ====================================================================== */