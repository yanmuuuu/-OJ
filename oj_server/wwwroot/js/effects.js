(function () {
    if (document.body.classList.contains('page-particles')) {
        initParticleTrail();
        return;
    }

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

function initParticleTrail() {
    var canvas = document.getElementById('particleCanvas');
    if (!canvas) return;

    var ctx = canvas.getContext('2d');
    var particles = [];
    var maxParticles = 72;
    var lastSpawn = 0;

    function resize() {
        canvas.width = window.innerWidth * devicePixelRatio;
        canvas.height = window.innerHeight * devicePixelRatio;
        canvas.style.width = window.innerWidth + 'px';
        canvas.style.height = window.innerHeight + 'px';
        ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    }

    function spawn(x, y, boost) {
        if (particles.length >= maxParticles) particles.shift();
        var angle = Math.random() * Math.PI * 2;
        var speed = boost ? 1.2 + Math.random() * 1.4 : 0.4 + Math.random() * 0.8;
        particles.push({
            x: x,
            y: y,
            vx: Math.cos(angle) * speed,
            vy: Math.sin(angle) * speed,
            life: boost ? 1 : 0.75 + Math.random() * 0.25,
            size: boost ? 2.5 + Math.random() * 2 : 1.8 + Math.random() * 2.2,
            hue: 200 + Math.random() * 45
        });
    }

    function tick() {
        ctx.clearRect(0, 0, window.innerWidth, window.innerHeight);

        for (var i = 0; i < particles.length - 1; i++) {
            var a = particles[i];
            var b = particles[i + 1];
            if (a.life < 0.15 || b.life < 0.15) continue;
            ctx.beginPath();
            ctx.moveTo(a.x, a.y);
            ctx.lineTo(b.x, b.y);
            ctx.strokeStyle = 'hsla(' + a.hue + ', 90%, 70%, ' + (Math.min(a.life, b.life) * 0.35) + ')';
            ctx.lineWidth = 1.2;
            ctx.stroke();
        }

        for (var j = particles.length - 1; j >= 0; j--) {
            var p = particles[j];
            p.x += p.vx;
            p.y += p.vy;
            p.vx *= 0.98;
            p.vy *= 0.98;
            p.life -= 0.022;
            if (p.life <= 0) {
                particles.splice(j, 1);
                continue;
            }
            ctx.beginPath();
            ctx.arc(p.x, p.y, p.size * p.life, 0, Math.PI * 2);
            ctx.fillStyle = 'hsla(' + p.hue + ', 88%, 72%, ' + (p.life * 0.7) + ')';
            ctx.fill();
        }

        requestAnimationFrame(tick);
    }

    document.addEventListener('mousemove', function (e) {
        var t = Date.now();
        if (t - lastSpawn < 16) return;
        lastSpawn = t;
        spawn(e.clientX, e.clientY, true);
        spawn(e.clientX + (Math.random() - 0.5) * 12, e.clientY + (Math.random() - 0.5) * 12, false);
    });

    window.addEventListener('resize', resize);
    resize();
    requestAnimationFrame(tick);
}
