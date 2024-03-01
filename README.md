# mdlv
Experimental sub-branch of cdlv. HTTP server implementation of cdlv with JS web client functioning as cdlv-menu alternative.

- Uses Mongoose for HTTP serving.
- Address-sanitized.
- Uses mp4 for animations

## cdlv scene features implemented
- [x] static backgrounds changed based on script file
- [x] looped animation
- [x] single loop animation
- [ ] prompt to jump to another scene based on player choice or scripted behavior

## usage
Refer to [makefile](makefile) for building. 

Change `path` and `host` inside [main.c](main.c) according to your needs. Daemonize if you're serious.

mdlv will scan `path` looking for `.adv` files. It will launch it's basic API and web client under `host`.

Parsing is done client-side in [script.js](static/script.js).
