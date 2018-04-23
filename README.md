# free-form-deformation
As my first coding exercise in JavaScript, I implemented the FFD algorithm in ffd.js. See Sederberg and Parry's '86 paper, Free-Form Deformation of Solid Geometric Models. The index.html contains my JavaScript code that consumes ffd.js and a modified version of the webgl_modifier_subdivision example from threejs.org.

Visit this URL to execute the script.
https://cdn.rawgit.com/glennchun/free-form-deformation/master/index.html

This works best on **Firefox** where all edges of the lattice are always displayed. Some edges disappear or reappear on **Chrome**, depending on how you rotate the view.