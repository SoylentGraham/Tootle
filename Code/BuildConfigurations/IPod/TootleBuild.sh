#!/bin/sh
# Tootle's automated build script;
# use: ./TootleBuild.sh Seaman

# assumes you've already checked out the repositories.
# Structure like
# ~/TootleBuild/
# add a symbolic link to this script in ~/TootleBuild/
# checkout Tootle, PROJECT etc to ~/TootleBuild/Tootle and ~/TootleBuild/PROJECT
# then call ./TootleBuild.sh PROJECT and it does the following

# Update svn
# Build 
# Zip
# Upload


PROJECT=$1

echo "Updating svn..."
svn update ~/TootleBuild/$PROJECT > ~/TootleBuild/${PROJECT}_svn.txt
if [ $? != 0 ]; then
	echo "svn update $PROJECT failed."
	exit 1
fi

# need to change directory for xcodebuild to work :(
echo "Building $PROJECT..."
cd ~/TootleBuild/$PROJECT/Code/IPod/
xcodebuild -configuration "Release AdHoc" -sdk iphoneos2.2.1 build > ~/TootleBuild/${PROJECT}_build.txt
if [ $? != 0 ]; then
	echo "xcode build failed. read ${PROJECT}_build.txt"
	exit 1
fi


# change directory to .app's directory
# this is to make things simple and so we're in our zip's directory
# so when we zip up we don't include a massive directory structure
cd "build/Release AdHoc-iphoneos/"
APP_FILENAME="$PROJECT.app"

# check .app has been built
if [ ! -e "$APP_FILENAME" ]; then
	CURRENT_PATH=`pwd`
	echo ".app doesn't exist - failed to build? ${CURRENT_PATH}/${PRODUCT_FILENAME}"
	exit 1
fi

# date in Fri_15May_1301 format appended to zip filename, after the .app and before the .zip
DATE_STRING=`date +%a_%d%b_%H%M`
ZIP_FILENAME="${APP_FILENAME}.${DATE_STRING}.zip"

# ftp details for curl
FTP_HOST="ftp.grahamreeves.com"
FTP_USER="graham70"
FTP_PASS="a5c59gra533"
FTP_DIR="public_html/tootlegames/beta/"


echo "Zipping up ${APP_FILENAME}..."
echo "Zip filename: ${ZIP_FILENAME}"
zip "${ZIP_FILENAME}" -r "${APP_FILENAME}" > ${PROJECT}_zip.txt

# abort if we failed to zip
if [ $? != 0 ]; then
	echo "Failed to zip ${ZIP_FILENAME}; see ${PROJECT}_zip.txt"
	exit 1
fi

# upload zip with curl
echo "uploading ${ZIP_FILENAME} to ${FTP_HOST}/${FTP_DIR}..."
curl -T "${ZIP_FILENAME}" -u ${FTP_USER}:${FTP_PASS} ftp://${FTP_HOST}/${FTP_DIR} > "${PROJECT}_ftp.txt"

# failed
if [ $? != 0 ]; then
	echo "Failed to upload ${ZIP_FILENAME}; see ${PROJECT}_ftp.txt"
	exit 1
fi

echo "Zip and upload finished. $PROJECT built!"
exit 0
