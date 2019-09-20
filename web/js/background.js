/*

Copyright 2019 Applied Research Center for Computer Networks

*/

Background = (function() {

    var imageUrl = '/api/v1/files/static/canvas-poster.jpg';
    var state = {
        isShown : true,
        isImageLoaded : false,
        image : null,

        x : 0,
        y : 0,

        isDrag : false,
        isDraggable : false,
        oldX   : 0,
        oldY   : 0
    };

    constructor();

    function constructor() {
        loadImage();

        state.x = parseInt(localStorage.getItem('background:x')) || 0;
        state.y = parseInt(localStorage.getItem('background:y')) || 0;

        window.addEventListener('ui:loaded', function() {

        });
    }

    function loadImage() {
        var image = new Image();
        image.onload = onLoadImage;
        image.src = imageUrl + '?rand=' + Math.random();
        state.image = image;
    }

    function draw() {
        if (!state.isImageLoaded || !state.isShown) return;

        CTX.save();
        CTX.globalAlpha = 0.7;
        CTX.drawImage(state.image, state.x, state.y);
        CTX.restore();
    }

    function reload() {

    }

    function hide() {
        state.isShown = false;
    }

    function show() {
        state.isShown = true;
    }

    function reset() {
        state.x = 0;
        state.y = 0;

        localStorage.setItem('background:x', state.x);
        localStorage.setItem('background:y', state.y);
    }

    function upload(file) {

        var xhr = new XMLHttpRequest();
        var formData = new FormData();

        xhr.onload = xhr.onerror = function() {
            loadImage();
        };

        formData.append('file', file);
        formData.append('static', 'canvas-poster.jpg');

        xhr.open('POST', '/api/v1/files', true);
        xhr.send(formData);

    }

    function setShown(isShown) {
        state.isShown = isShown;
    }
    function setDraggable(isDraggable) {
        state.isDraggable = isDraggable;
        if (isDraggable) {
            UI.canvas.addEventListener('mousedown', onMouseDown);
            UI.canvas.addEventListener('mouseup', onMouseUp);
        } else {
            UI.canvas.removeEventListener('mousedown', onMouseDown);
            UI.canvas.removeEventListener('mouseup', onMouseUp);
        }
    }

    function isDraggable() {
        return state.isDraggable;
    }
    function isShown() {
        return state.isShown;
    }

// Events

    function onLoadImage() {
        state.isImageLoaded = true;
    }

    function onMouseMove(e){
        if (state.isDrag) {
            state.x += e.pageX - state.oldX;
            state.y += e.pageY - state.oldY;

            console.log(state.y, e.pageY, state.oldY);

            localStorage.setItem('background:x', state.x);
            localStorage.setItem('background:y', state.y);

            state.oldX = e.pageX;
            state.oldY = e.pageY;
        }
    }

    function onMouseDown(e){
        if (!state.isDraggable) return;

        state.oldX = e.pageX;
        state.oldY = e.pageY;

        state.isDrag = true;
        UI.canvas.addEventListener('mousemove', onMouseMove);
    }

    function onMouseUp(){
        state.isDrag = false;
        UI.canvas.removeEventListener('mousemove', onMouseMove);
    }

// Events END

    return {
        draw : draw,
        hide : hide,
        show : show,
        reset   : reset,
        upload  : upload,

        isDraggable : isDraggable,
        isShown     : isShown,

        setDraggable    : setDraggable,
        setShown        : setShown,

        // for debug
        state : state
    }
})();
