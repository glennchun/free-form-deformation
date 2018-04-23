# free-form-deformation
As my first coding exercise in JavaScript, I implemented the FFD algorithm in ffd.js. See Sederberg and Parry's '86 paper, Free-Form Deformation of Solid Geometric Models. The index.html contains my JavaScript code that consumes ffd.js and a modified version of the webgl_modifier_subdivision example from threejs.org.

This branch, wasm-test, contains my experiment with WebAssembly. Before you view the web application at http://localhost:9000, execute the following command in Python 2.7: python http_server.py

This application works best on **Firefox** where all edges of the lattice are always displayed. Some edges disappear or reappear on **Chrome**, depending on how you rotate the view.