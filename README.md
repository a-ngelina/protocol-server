This is a protocol server that supports the following requests:
- STATUS
- QUIT
- LIST path/to/directory
- GET path/to/file
- POST path/to/file\ncontents (overwrites the file or creates a new file)
---
Requirements:
c++20 or later
---
Build:
You can compile with -DDEBUG flag for additional info on requests/responses during runtime
In the tests directory, you'll find some sample tests as well as a script to automatically compile and run all the tests available, though you're free to add yours
