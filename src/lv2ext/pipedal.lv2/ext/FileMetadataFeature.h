/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#ifndef PIPEDAL_FILE_METADAtA_FEATURE_H
#define PIPEDAL_FILE_METADAtA_FEATURE_H
#include "lv2/core/lv2.h"
#include "lv2/urid/urid.h"

#define PIPEDAL__FILE_METADATA_FEATURE "http://github.com/rerdavies/pipedal/ext/#fileMetadata"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef void *PIPEDAL_FILE_METADATA_Handle;

    typedef enum
    {
        PIPEDAL_FILE_METADATA_SUCCESS = 0,           /**< Completed successfully. */
        PIPEDAL_FILE_METADATA_INVALID_PATH = 1,      /**< Path is not in Tracks directory, or does not exist. */
        PIPEDAL_FILE_METADATA_PERMISSION_DENIED = 2, /**< Permission denied. */
        PIPEDAL_FILE_METADATA_ERR_UNKNOWNM = 3,      /**< Unknown error. */
        PIPEDAL_FILE_METADATA_INVALID_KEY = 4,       /**< Key is not a valid URID. */
        PIPEDAL_FILE_METADATA_NOT_FOUND = 5,         /**< Metadata not found. */
    } PIPEDAL_FileMetadata_Status;

    typedef struct
    {
        /**
            Opaque host data.
        */
        PIPEDAL_FILE_METADATA_Handle handle;

        /**
           Save a piece of plugin-defined metadata for a file.
          @param handle MUST be the `handle` member of this struct.
          @param absolute_path The absolute path of a file.
          @param key A plugin-defined key (LV2_URID) used to identify the metdadata.
          @param metdata A string containing the metadata to be saved for the file.
          @return A status code indicating success or failure.

          The path must be within the host-defined "Tracks" directory. The file must exist. The plugin does not
          need write access to the directory containing the file in order to save metadata.

          Plugins MUST NOT make any assumptions about abstract paths except that
          they can be mapped back to the absolute path of the "same" file (though
          not necessarily the same original path) using absolute_path().

          Metatadata will be automatically deleted if the file is deleted, of the file is modified, or moved
          to another directory.

          This function should not be called from the plugin's realtime thread, as it may block for a long time.
      */

        PIPEDAL_FileMetadata_Status (*setFileMetadata)(
            PIPEDAL_FILE_METADATA_Handle handle,
            const char *absolute_path,
            const char *key,
            const char *fileMetadata);
        /**
           Restore previously saved metdata for the file.
          @param handle MUST be the `handle` member of this struct.
          @param absolute_path The absolute path of a file.
          @param key A plugin-defined key used to identify the metdadata for a particular appliction.
          @param buffer A buffer in which to store the metadata.
          @param buffersize The size of the buffer in bytes, including space for the null terminator.
          @return A value indicating the number of bytes written to the buffer, or 0 if the metdata was not found.

          If buffersize is insufficient to holed the metatdata, the function will return the number of bytes that would have been written,
          including the space for the null terminator, but will not write any data to the buffer. The best way to find out
          how much space is needed is to call this function with a buffer size of 0, which will return the size needed to contain the
          metadata result.

          This function should not be called from the plugin's realtime thread, as it may block for a long time.

      */
        uint32_t (*getFileMetadata)(
            PIPEDAL_FILE_METADATA_Handle handle,
            const char *filePath,
            const char *key,
            char *buffer,
            uint32_t bufferSize);
        /**
           Restore previously saved metdata for the file.
          @param handle MUST be the `handle` member of this struct.
          @param absolute_path The absolute path of a file.
          @param key A plugin-defined key used to identify the metdadata for a particular appliction.
          @return A status code indicating success or failure.
          This function will delete the metadata for the file, if it exists. If the file does not exist,
          or is not in the Tracks directory, it will return PIPEDAL_FILE_METADATA_INVALID_PATH.
        */

        PIPEDAL_FileMetadata_Status (*deleteFileMetadata)(
            PIPEDAL_FILE_METADATA_Handle handle,
            const char *filePath,
            const char *key);
    } PIPEDAL_FileMetadata_Interface;

    typedef struct
    {
        const char *URI;
        PIPEDAL_FileMetadata_Interface *interface;
    } PIPEDAL_FileMetadata_Feature;

#ifdef __cplusplus
}
#endif

#endif // PIPEDAL_FILE_METADAtA_FEATURE_H