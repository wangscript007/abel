
#ifndef ABEL_IO_TEMP_FILE_H_
#define ABEL_IO_TEMP_FILE_H_

#include <abel/base/profile.h>

namespace abel {

    // Create a temporary file in current directory, which will be deleted when
    // corresponding TempFile object destructs, typically for unit testing.
    //
    // Usage:
    //   {
    //      TempFile tmpfile;           // A temporay file shall be created
    //      tmpfile.save("some text");  // Write into the temporary file
    //   }
    //   // The temporary file shall be removed due to destruction of tmpfile

    class temp_file {
    public:
        // Create a temporary file in current directory. If |ext| is given,
        // filename will be temp_file_XXXXXX.|ext|, temp_file_XXXXXX otherwise.
        // If temporary file cannot be created, all save*() functions will
        // return -1. If |ext| is too long, filename will be truncated.
        temp_file();

        explicit temp_file(const char *ext);

        // The temporary file is removed in destructor.
        ~temp_file();

        // Save |content| to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save(const char *content);

        // Save |fmt| and associated values to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save_format(const char *fmt, ...) __attribute__((format (printf, 2, 3)));

        // Save binary data |buf| (|count| bytes) to file, overwriting existing file.
        // Returns 0 when successful, -1 otherwise.
        int save_bin(const void *buf, size_t count);

        // Get name of the temporary file.
        const char *fname() const { return _fname; }

    private:
        // TempFile is associated with file, copying makes no sense.
        ABEL_NON_COPYABLE(temp_file);

        int _reopen_if_necessary();

        int _fd;                // file descriptor
        int _ever_opened;
        char _fname[24];        // name of the file
    };

} // namespace abel

#endif  // ABEL_IO_TEMP_FILE_H_
