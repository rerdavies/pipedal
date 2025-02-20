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

   Because PiPedal is accessed remotely through a web interface, audio and model files 
   have to be uploaded into standardized directories using the Upload File button, found in 
   the File Property Dialog. Plugins that run locally can place sample and model files 
   more casually, since users can browse anywhere on their computer using the system 
   file dialog.

   This LV2 Extension allows plugin developers to determine the location of well-known directory.
   
   The extension addresses three  primary uses-cases:

   - Allow plugins to published factory-installed resource files that can be 
     selected using the Patch Property file dialog. Typically. Source files would be 
     stored in the bundle directory of the plugin at install time. On first use, the
     plugin would create symlinks in the well-known directory that reference files in the 
     bundle directory.
   
   - Allow plugins to reference factory-installed resource files that can be selected using 
     the Patch Property file dialog.

   - Allow recording plugins to locate the directory in which new files should be
     placed.

   The plugin recognizes the following well-known directory locations, which are based on MOD fileType 
   conventions.

   "audioloop"
        Audio Loops, meant to be used for looper-style plugins.
   "audiorecording",
         Audio Recordings, triggered by plugins and stored in the unit
   "audiosample",
        One-shot Audio Samples, meant to be used for sampler-style plugins
   "audiotrack"
        Audio Tracks, meant to be used as full-performance/song or backtrack
   "cabsim"
       Impulse response files for convolution-based Cabinet response simulation.
   "h2drumkit"
       Hydrogen Drumkits, must use h2drumkit file extension. Not currently supported by PiPedal.
   "ir"
       Impulse response files of arbitrary size. May be either CAB IR files, or Reverbe IR files. 
   "midiclip"
      MIDI Clips", {".mid", ".midi"}}, // MIDI Clips, to be used in sync with
      host tempo, must have mid or midi file extension
   "midisong"
      MIDI Songs", {".mid", ".midi"}}, meant to be used as
      full-performance/song or backtrack.
   "sf2"
      SF2 Instruments, must have sf2 or sf3 file extension
   "sfz"
      SFZ Instruments, must have sfz file extension
   "audio"
      Parent directory of all audio file directories. Not officially documented
      by MOD, but has been observed in the field.
   "nammodel"
      .nam files: Nueral amp model's for Steven Atkin's Neural Amp Modeler library.
   "aidadspmodel"
      AIDA IAX model files. (file extensions of ".json")
   "mlmodel"
      Model files for plugins based on the ML neural net library. (file
      extensions of ".json"). TooB ML, Proteus, probably others.
   "~"
      References a directory that is for private use by only the current plugin.
      Users can upload and select files in the private directory when using
      the current plugin, but no other plugins can access the files. (Not
      supported by MOD).


   Methods are are safe on the realtime thread, as they perform file i/o and allocate memory. This feature should be used 
   at initialization time, in the call to Activate, or else on a scheduler thread.

   @{
*/


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
   LV2_FileBrowser_Status_Success = 0,
   LV2_FileBrowser_Status_Err_InvalidParameter = 1,
   LV2_FileBrowser_Status_Err_Filesystem = 2,
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

  /**
     Publish a resource file provided by the plugin to a well-known directory location.

     @param handle MUST be the `handle` member of this struct. 

     @param version The version number of the resource file.

     @param resourcePath The source filename./

     @param uploadDirectory The target filename.

     @return LV2_FileBrowser_Status Zero if successful.

     The version number is the version number of the resource file(s). The host keeps track of which 
     resource files have previously been installed, and the version thereof. Files will only be 
     published once. This allows users to delete published files if they want to; and allows 
     upgraded plugins to write upgraded resource files. 

     `resourcePath` is specified relative to the plugin's bundle directory. You can supply an 
     absolute path if you want, but it's generally better to place resource files in the plugin's
     bundle, and specify the location with a relative path. If the resourcePath specifies a file, 
     only the file will be copied; if the the path specifies a directory, all files and subdirectories
     will be published.
     
     `uploadDirectory` most be a relative path of a directory, and the first segment of the path must be one of
     the well-known directory locations. 

     A return code of LV2_FileBrowser_Status_Err_InvalidParameter indicates
     either that the well-known directory was not found, or that an absolute
     path was provided where a relative path was expected.
     LV2_FileBrowser_Status_Err_Filesystem will be return if some kind of
     catastrophic filesystem error occurred (out of disk space, perhaps, or
     permission denied).
     
      Example: 
      [code] 
           // becomes /usr/lib/lv2/ToobAmp.lv2/namFactoryModels/FenderBlackface.nam (for example) 
          const char*sourceFile = "namFactoryModels/Fender Blackface.nam"

          // /becomes /var/pipedal/audio_uploads/NeuralAmpModels/TooB NAM (for example)
          const char(*targetDirectory = "namModel/TooB NAM")

          Lv2_FileBrowserStatus ec = 
              feature.publish_resource_files(feature.handle,1,sourceFile,targetDirectory);

         // on success creates a symlink at 
         // /var/pipedal/audio_uploads/NeuralAmpModels/TooB NAM/Fender Blackface.nam
     [/code]
  */

   LV2_FileBrowser_Status (*publish_resource_files)(
      LV2_FileBrowser_Files_Handle handle,
      uint32_t version,
      const char*resourcePath, 
      const char*uploadDirectory);

  /**
     ccc

     @param handle MUST be the `handle` member of this struct. 

     @param path A relative path, starting with a well-known directory name.

     @return The absolute path to use for the new directory.

     This function can be used by plugins to get the path of files or directories
     that can be browsed by file dialogs. get_filebrowser_path, unlike
     map_path() does not create or modify files or directories in any way.

     The path can have multiple segments, but the first segment of the path must be 
     the name of a well-known directory.

     The caller must free memory for the returned value with
     LV2_FileBrowser_Files.free_path().

     Example:
     [code]
         const char *filePath = feature.get_upload_path(feature.handle,"audio/Tracks/Xmas/Frosty.wav");
         // returns (for example)
         // /var/pipedal/audio_uploads/shared/audio/Tracks/Xmas/Frosty.wav
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
     directory and returns the path of that link.
     
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
