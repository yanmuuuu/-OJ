(function () {
    const nav = document.getElementById('navAuth');
    if (!nav) return;

    function renderLoggedOut() {
        nav.innerHTML =
            '<a class="navbar-link" href="/login">登录</a>' +
            '<a class="navbar-link navbar-link-accent" href="/register">注册</a>';
    }

    function renderLoggedIn(nickname) {
        nav.innerHTML =
            '<a class="navbar-link" href="/my_progress">我的做题</a>' +
            '<a class="navbar-link" href="/submit_question">录题</a>' +
            '<span class="navbar-user">' + escapeHtml(nickname) + '</span>' +
            '<button type="button" class="navbar-logout" id="navLogout">退出</button>';
        const btn = document.getElementById('navLogout');
        if (btn) {
            btn.addEventListener('click', async function () {
                try {
                    await fetch('/api/logout', {
                        method: 'POST',
                        credentials: 'same-origin',
                        body: ''
                    });
                } catch (e) { /* ignore */ }
                window.location.href = '/all_questions';
            });
        }
    }

    function escapeHtml(s) {
        const d = document.createElement('div');
        d.textContent = s;
        return d.innerHTML;
    }

    fetch('/api/me', { credentials: 'same-origin' })
        .then(function (r) { return r.json(); })
        .then(function (data) {
            if (data.data && data.data.nickname) {
                renderLoggedIn(data.data.nickname);
            } else if (data.data && data.data.username) {
                renderLoggedIn(data.data.username);
            } else {
                renderLoggedOut();
            }
        })
        .catch(function () {
            renderLoggedOut();
        });
})();
