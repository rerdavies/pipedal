/*
 *   Copyright (c) 2023 Robin E. R. Davies
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

#ifndef FILEBROWSERFILES_H
#define FILEBROWSERFILES_H

#define LV2_FILEBROWSER_URI "http://two-play.com/ns/ext/fileBrowser"
#define LV2_FILEBROWSER_PREFIX LV2_FILEBROWSER_URI "#"
#define LV2_FILEBROWSER__files LV2_FILEBROWSER_PREFIX "files"           ///< http://two-play.com/ns/ext/fileBrowser#files >

/**
   @defgroup filedialog LV2_FileBrowser_Files
   @ingroup lv2

   A feature for LV2 plugins to provide sample files (or other selectable files) within the plugin's resource bundle to host file selection dialogs.

   Browser directories may contain actual files that typically have been uploaded by users; but may also contain symlinks to files
   in other locations. This feature allows plugins to publish files within the plugin's resource bundle to browser directories. For 
   each published file consists of a symlink in the browser directory that links to a corresponding file in the plugins resource bundle.

   The LV2_FileBrowser_Files.publish_resource_files() method allows plugins to publish entire directories. publish_resource_files() 
   implements a versioning system, which tracks whether sample files have been deleted by the user. If the symlink for sample files that were installed
   by a previous version of the plugin have been deleted, the sample files from the current version are not reinstalled. See LV2_FileBrowser_Files.publish_resource_files.
   not re-installed.

   See: <http://rerdavies.github.io/pipedal/TBD XXX.html> for more details.

   @{
*/


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
   LV2_FileBrowser_Status_Success = 0,
   LV2_FileBrowser_Status_Err_Filesystem = 1,
   LV2_FileBrowser_Status_Err_InvalidParameter = 1,
} LV2_FileBrowser_Status;

typedef void*
  LV2_FileBrowser_Files_Handle; ///< Opaque handle for pipedal:uploadPath feature


/**
   Feature data for filedialog:files (@ref LV2_FILEBROWSER_file).
*/
typedef struct {
  /**
     Opaque host data.
  */
   LV2_FileBrowser_Files_Handle handle;

   LV2_FileBrowser_Status (*publish_resource_files)(LV2_FileBrowser_Files_Handle handle,uint32_t version,const char*resourcePath, const char*uploadDirectory);

  /**
     Return a path in which upload files will be stored for this plugin.
     @param handle MUST be the `handle` member of this struct.
     @param path The sub-directory name in which uploaded files will be stored.
     @return The absolute path to use for the new directory.

     This function can be used by plugins to get the path of files or directories
     that can be browsed by file dialogs. get_filebrowser_path, unlike map_path() does
     not create or modify files in any way.

     The caller must free memory for the returned value with LV2_FileBrowser_Files.free_path().

     Example:
     [code]
         TBD
     [/code]
  */
  char* (*get_upload_path)(LV2_FileBrowser_Files_Handle handle, const char* path);

  /**
     Map resource bundle filenames to upload directories.
     @param handle MUST be the `handle` member of this struct.
     @param path File path name
     @param fileBrowserDirectory relative directory of the file browser directory in which to create a link.
     @return The absolute pathname of a file in the resource directory.

     The primary purpose of this method is to map file references to files in the plugin's bundle
     directory to corresponding files in the a file browser directory.

     If the path is relative, the full path is resolved using the `rootResourceDirectory` path.

     If the full path is not a child of rootResourceDirectory or if the path is relative,
     get_upload_path returns the supplied path unmodified.
`
     If the filename is a child of rootResourceDirectory, creates a corresponding link to the file in the file browser 
     directory and returns the name of that link.
     
     The caller must free memory for the returned value with `LV2_FileBrowser_Files.free_path()`.

     Example:
  */
  char* (*map_path)(LV2_FileBrowser_Files_Handle handle, const char* path, const char*rootResourceDirectory, const char*fileBrowserDirectory);


  /**
    Free a path returned by LV2_FileBrowser_Files.get_upload_path() or LV2_FileBrowser_Files.map_path()
     @param handle MUST be the `handle` member of this struct.
     @param path A path previously returned by a method of `LV2_FileBrowser_Files`.

  */
  void (*free_path)(LV2_FileBrowser_Files_Handle handle, char* path);
} LV2_FileBrowser_Files;



#ifdef __cplusplus
} // extern "C"
#endif

#endif // FILEBROWSERFILES_H
