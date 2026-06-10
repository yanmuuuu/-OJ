(function () {
    const form = document.getElementById('loginForm') || document.getElementById('registerForm');
    if (!form) return;

    const mode = form.dataset.mode;
    const errorEl = document.getElementById('authError');
    const submitBtn = document.getElementById('authSubmit');

    function showError(msg) {
        if (!errorEl) return;
        errorEl.textContent = msg;
        errorEl.classList.remove('hidden');
    }

    function hideError() {
        if (!errorEl) return;
        errorEl.classList.add('hidden');
    }

    form.addEventListener('submit', async function (e) {
        e.preventDefault();
        hideError();

        const username = form.querySelector('[name="username"]').value.trim();
        const password = form.querySelector('[name="password"]').value;

        let url = '/api/login';
        let body = { username, password };

        if (mode === 'register') {
            const confirm = form.querySelector('[name="confirm_password"]').value;
            url = '/api/register';
            body = { username, password, confirm_password: confirm };
        }

        submitBtn.disabled = true;
        submitBtn.textContent = mode === 'register' ? '注册中…' : '登录中…';

        try {
            const resp = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json;charset=utf-8' },
                body: JSON.stringify(body),
                credentials: 'same-origin'
            });
            const data = await resp.json();

            if (data.errcode !== 0) {
                showError(data.errmsg || '操作失败');
                return;
            }

            if (mode === 'register') {
                window.location.href = '/login';
            } else {
                window.location.href = '/all_questions';
            }
        } catch (err) {
            showError('网络错误，请稍后重试');
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = mode === 'register' ? '注册' : '登录';
        }
    });
})();
