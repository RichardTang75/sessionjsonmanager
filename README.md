# sessionjsonmanager

Session replacer for Mozilla Firefox. In progress. 

Uses wxwidgets, nlohmann json (https://github.com/nlohmann/json), lz4 (https://github.com/lz4/lz4), and possibly boost filesystem to copy firefox's sessionstore-backup jsonlz4s to another folder and then replaces the sessionstore's backup. This allows the usage of Firefox's own session restore mechanism (which it constantly writes to) in order to use as a session manager. 
