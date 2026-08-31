# File Manager

## Requirements
1. File manager supports fetching a Page from disk, given a `pageId`
2. File manager supports persisting new pages/changes to existing pages
3. File manager supports deleting a page and reclaiming a slot in the page
4. File manager supports flushing changes to synchronize data with the storage device
5. File manager supports constructing pages from an existing database file on start up
6. File manager supports extending the database file to allow for additional pages

## Design

### File Manager Structure
- File Manager is a central wrapper for multiple File Managers. Each File Manager is responsible for 
managing

### File Manager Construction
On File Manager construction, we need to load the DB file. If it is not empty, we need to get the file
size to initialize `numPages`

```C++
FileManager() {
    fs.open(DATABASE_FILE_NAME, std::fstream::in | std::fstream::out | std::fstream::app);
    
    if (!fs.is_open()) {
        // exit
    }
    
    int fileSize;
    // get file size through seekg() and tellg()
    
    numPages = fileSize/PAGE_SIZE
}
```