echo "Starting file copy..."

set SOURCE_DIR="C:\Users\sebas\source\projects\Handmade_Game"
set TARGET_DIR="C:\Users\sebas\source\github\accounts\shyjek123\Handmade_Game"

# Using rsync for silent copying, but you can use cp with -r and --quiet as a less robust alternative.
#rsync -a --quiet --exclude "sync_folders.sh" "$SOURCE_DIR/" "$TARGET_DIR/"
dir
echo "Files copied from %SOURCE_DIR% to %TARGET_DIR%"
