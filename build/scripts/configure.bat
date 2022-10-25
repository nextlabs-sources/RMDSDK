REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
REM % configure.bat
REM %
REM % Configure various variables for making builds on feature branches,
REM % develop branch, release branch, and hotfix branch.
REM %
REM % Usage: configure
REM %
REM % The hard-coded values of the following variables in this script
REM % must be manually adjusted When a branch is made:
REM % - CONFIG_TYPE
REM % - FEATURE_NAME
REM % - BRANCH_ID
REM %
REM % (This script is analogus to the script "configure" in the top of
REM % Fate repository.  However, all choices are hard-coded inside
REM % this script instead of being passed in command-line.  This makes
REM % things simpler.)
REM %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



REM
REM CONFIG_TYPE must be one of the following:
REM - feature_smdc (feature/XXX branch for SMDC)
REM - feature_cdc (feature/XXX branch for CDC)
REM - develop (develop branch)
REM - release (other branch)
REM 
set CONFIG_TYPE=release



REM
REM FEATURE_NAME must be one of the following:
REM - MyFeatureName (feature/XXX branch)
REM - <blank> (other branch)
REM
set FEATURE_NAME=



REM 
REM BRANCH_ID must be one of the following:
REM - An arbitary number higher than 500 that is unique among all feature
REM   branches in the same repo (feature/XXX branch) (In practice, we just
REM   choose the first ununsed number above 500.)
REM - <blank> (other branch)
REM
REM (This serves the same purpose as the overloaded VERSION_PATCH_XXX in
REM build/Makefile.ver in PCV branches in Fate repo.  Please see
REM Section 4.2 "PCV Build" sub-section "Version String", as well as
REM Section 6 "Versioning" sub-section "PCV Build Versioning", in
REM https://sharepoint.nextlabs.com/sites/yoda/build/Documents/NextLabs%20Software%20Build%20Infrastructure%20Design%20Document%202012%20v0.1.zip.)
REM
set BRANCH_ID=



REM
REM BUILD_CODE must be one of the following:
REM - FS (feature/XXX branch for SMDC)
REM - FC (feature/XXX branch for CDC)
REM - <blank> (other branch)
REM
IF "%CONFIG_TYPE%"=="feature_smdc" (
  set BUILD_CODE=FS
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set BUILD_CODE=FC
) ELSE (
  set BUILD_CODE=
)



REM
REM VERSION_BUILD_SHORT must be one of the following:
REM - <build number><build code>_<feature name> (feature/XXX branch)
REM - <build number> (other branch)
REM
IF "%CONFIG_TYPE%"=="feature_smdc" (
  set VERSION_BUILD_SHORT=%BUILD_NUMBER%%BUILD_CODE%_%FEATURE_NAME%
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set VERSION_BUILD_SHORT=%BUILD_NUMBER%%BUILD_CODE%_%FEATURE_NAME%
) ELSE (
  set VERSION_BUILD_SHORT=%BUILD_NUMBER%
)



REM
REM OUTPUT_REPOSITORY_ROOT must be one of the following:
REM - build\feature (feature/XXX branch)
REM - build\develop (develop branch)
REM - build\release_candidate (other branch)
REM (This is analogus to REPOSITORY_ROOT in configure in Fate repo.)
REM
IF "%CONFIG_TYPE%"=="feature_smdc" (
  set OUTPUT_REPOSITORY_ROOT=build\feature
  set OUTPUT_REPOSITORY_ROOT_POSIX=build/feature
) ELSE IF "%CONFIG_TYPE%"=="feature_cdc" (
  set OUTPUT_REPOSITORY_ROOT=build\feature
  set OUTPUT_REPOSITORY_ROOT_POSIX=build/feature
) ELSE IF "%CONFIG_TYPE%"=="develop" (
  set OUTPUT_REPOSITORY_ROOT=build\develop
  set OUTPUT_REPOSITORY_ROOT_POSIX=build/develop
) ELSE (
  set OUTPUT_REPOSITORY_ROOT=build\release_candidate
  set OUTPUT_REPOSITORY_ROOT_POSIX=build/release_candidate
)
