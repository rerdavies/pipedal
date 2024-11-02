## Release Checklist

Steps to publishing a release. Chances should be waiting on a dev branch.

### prepare the commit.

- Update versions. In vscode, do a global replace of the previous version with the next. Close ReleaseNotes.md to ignore.
    There should be 13 replacements (excluding ReleaseNotes.md).
- Make sure the verision display name has the correct suffix (-Release, -Beta or -Experimental).
- Write release notes for the new version in ReleaseNotes.md
- On the dev machine do a full relWithDebugInfo build. (symbols will be stripped in the package)
- On the dev machine, run `./buildPackage`
- On the dev machine, run `./signPackage`
- Commit all changes on the dev branch. 
- Wait for the Github build action for the dev branch to complete. There should be no build errors.

### preparing DRAFT release on github.

- On git hub, create a DRAFT release. 
- Release Title format is "PiPedal v1.3.36 Release" (or "Beta" or "Experimental").
- Copy release notes from ReleaseNotes.md into the draft release.
- Copy footnotes (link to full release notes, and PGP info) from the previous release.
- Set tag to "create a new tag" with appropriate version name. e.g. "v1.3.26" (no tag suffix)
- UPLAOD THE DEBIAN PACKAGE AND ITS .asc filtering
- Check "Create a discussion group".
- Set "Set as current release" ONLY for Release and Beta builds.
- Do NOT COMMIT the DRAFT yet. Wait until the dev branch has been merged in following steps.

### Merging the changes.
- Check that the dev build action completed without errors.
- MERGE the dev branch onto the main branch.
- Without waiting for build actions to complete, commit the github release, wich will now pick up the corect source bundles. The release is now live at this point.
- Docs should update 30 seconds after the  merge, when Github docs action completes.
- Verify that the main build and docs actions complete without errors.

### Post release verification.
- Verify that README.md links to the correct verion.
- Verify that online docs link to the correct version.
- Proofread the Release and associated notes.

Experimental releases should be built on the dev branch (and therefore do not show up in readme.md and docs).
