(function () {
    var spotlight = document.getElementById('spotlight');
    if (!spotlight) return;

    var visible = true;
    var x = window.innerWidth / 2;
    var y = window.innerHeight / 2;

    function paint() {
        spotlight.style.setProperty('--mx', x + 'px');
        spotlight.style.setProperty('--my', y + 'px');
    }

    document.addEventListener('mousemove', function (e) {
        x = e.clientX;
        y = e.clientY;
        if (!visible) {
            visible = true;
            spotlight.classList.remove('spotlight-off');
        }
        paint();
    });

    document.addEventListener('mouseleave', function () {
        visible = false;
        spotlight.classList.add('spotlight-off');
    });

    paint();
})();
