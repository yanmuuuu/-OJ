(function () {
    var ACE_BASE = 'https://cdn.jsdelivr.net/npm/ace-builds@1.32.0/src-min-noconflict';
    var DRAFT_KEY = 'oj_submit_draft_v1';
    var DRAFT_DEBOUNCE_MS = 600;

    var form = document.getElementById('submitQuestionForm');
    if (!form) return;

    var errorEl = document.getElementById('submitError');
    var submitBtn = document.getElementById('submitBtn');
    var fillExampleBtn = document.getElementById('fillExampleBtn');
    var undoExampleBtn = document.getElementById('undoExampleBtn');
    var exampleHint = document.getElementById('exampleHint');
    var draftBadge = document.getElementById('draftBadge');
    var openFullPreviewBtn = document.getElementById('openFullPreview');
    var previewModal = document.getElementById('previewModal');
    var previewBackdrop = document.getElementById('previewBackdrop');
    var closePreviewBtn = document.getElementById('closePreview');

    var pvTitle = document.getElementById('pvTitle');
    var pvAuthor = document.getElementById('pvAuthor');
    var pvDesc = document.getElementById('pvDesc');
    var pvHead = document.getElementById('pvHead');
    var pvMeta = document.getElementById('pvMeta');

    var modalTitle = document.getElementById('modalTitle');
    var modalMeta = document.getElementById('modalMeta');
    var modalDesc = document.getElementById('modalDesc');

    var FIELD_NAMES = ['title', 'star', 'desc', 'head', 'tail', 'cpu_limit', 'mem_limit'];

    var EXAMPLE = {
        title: '两数之和',
        star: '简单',
        desc: [
            '给定一个整数数组 nums 和一个整数目标值 target，请你在该数组中找出和为目标值 target 的那两个整数，并返回它们的数组下标。',
            '',
            '你可以假设每种输入只会对应一个答案，并且你不能使用两次相同的元素。',
            '',
            '示例 1：',
            '输入：nums = [2,7,11,15], target = 9',
            '输出：[0,1]',
            '',
            '示例 2：',
            '输入：nums = [3,2,4], target = 6',
            '输出：[1,2]',
            '',
            '提示：',
            '2 <= nums.length <= 10^4',
            '-10^9 <= nums[i], target <= 10^9'
        ].join('\n'),
        head: [
            '#include <bits/stdc++.h>',
            '',
            'using namespace std;',
            '',
            'class Solution {',
            'public:',
            '    vector<int> twoSum(vector<int>& nums, int target) {',
            '        // 在此填写代码',
            '    }',
            '};'
        ].join('\n'),
        tail: [
            '#ifndef COMPILER_ONLINE',
            '#include "head.cpp"',
            '#endif',
            '',
            'void test1() {',
            '    Solution s;',
            '    vector<int> nums = {2, 7, 11, 15};',
            '    int target = 9;',
            '    vector<int> result = s.twoSum(nums, target);',
            '    bool ok = (result.size() == 2 && result[0] != result[1] &&',
            '               nums[result[0]] + nums[result[1]] == target);',
            '    cout << (ok ? "test1 passed" : "test1 not passed") << endl;',
            '}',
            '',
            'void test2() {',
            '    Solution s;',
            '    vector<int> nums = {3, 3};',
            '    int target = 6;',
            '    vector<int> result = s.twoSum(nums, target);',
            '    bool ok = (result.size() == 2 && result[0] != result[1] &&',
            '               nums[result[0]] + nums[result[1]] == target);',
            '    cout << (ok ? "test2 passed" : "test2 not passed") << endl;',
            '}',
            '',
            'int main() {',
            '    test1();',
            '    test2();',
            '    return 0;',
            '}'
        ].join('\n'),
        cpu_limit: 1,
        mem_limit: 30000
    };

    var snapshotBeforeExample = null;
    var currentAuthor = '我';
    var draftTimer = null;
    var previewEditor = null;
    var aceLoaded = false;

    function escapeHtml(s) {
        return String(s).replace(/[&<>"']/g, function (c) {
            return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' }[c];
        });
    }

    function enhanceDescElement(el, raw) {
        if (!el) return;
        var text = raw !== undefined ? raw : el.textContent;
        var html = escapeHtml(text);
        html = html.replace(/`([^`]+)`/g, '<span class="inline-code">$1</span>');
        html = html.replace(/\b(nums|target|vector|int|string)\b/g, '<span class="inline-code">$1</span>');
        el.innerHTML = html;
    }

    function readFormState() {
        var state = {};
        FIELD_NAMES.forEach(function (name) {
            var el = form.querySelector('[name="' + name + '"]');
            state[name] = el ? el.value : '';
        });
        return state;
    }

    function writeFormState(state) {
        FIELD_NAMES.forEach(function (name) {
            var el = form.querySelector('[name="' + name + '"]');
            if (el && state[name] !== undefined)
                el.value = state[name];
        });
        updatePreview();
    }

    function isFormEmpty(state) {
        return FIELD_NAMES.every(function (name) {
            var v = state[name];
            if (name === 'cpu_limit') return v === '1' || v === '';
            if (name === 'mem_limit') return v === '30000' || v === '';
            return !String(v).trim();
        });
    }

    function truncate(text, max) {
        if (!text) return '';
        text = text.trim();
        if (text.length <= max) return text;
        return text.slice(0, max) + '…';
    }

    function buildMetaHtml(star, cpu, mem, author) {
        var starCls = star ? 'difficulty ' + star : 'difficulty preview-star-placeholder';
        var starText = star || '难度';
        return (
            '<span class="' + starCls + '">' + escapeHtml(starText) + '</span>' +
            '<span class="meta-tag meta-author">出题 · ' + escapeHtml(author || '我') + '</span>' +
            '<span class="meta-tag">TIME ' + escapeHtml(String(cpu || '1')) + 's</span>' +
            '<span class="meta-tag">MEM ' + escapeHtml(String(mem || '30000')) + 'MB</span>'
        );
    }

    function getFormData() {
        var state = readFormState();
        return {
            title: state.title.trim(),
            star: state.star,
            desc: state.desc,
            head: state.head,
            tail: state.tail,
            cpu_limit: state.cpu_limit || '1',
            mem_limit: state.mem_limit || '30000'
        };
    }

    function updatePreview() {
        var data = getFormData();
        if (pvTitle)
            pvTitle.textContent = data.title || '（未填写）';
        if (pvAuthor)
            pvAuthor.textContent = '出题 · ' + currentAuthor;
        if (pvMeta)
            pvMeta.innerHTML = buildMetaHtml(data.star, data.cpu_limit, data.mem_limit, currentAuthor);
        if (pvDesc)
            pvDesc.textContent = data.desc.trim() ? truncate(data.desc, 220) : '填写题目描述后将在此显示摘要…';
        if (pvHead) {
            var headText = data.head.trim() ? truncate(data.head, 480) : '// 填写 head 后预览';
            pvHead.innerHTML = '<code>' + escapeHtml(headText) + '</code>';
        }
        if (previewEditor)
            previewEditor.setValue(data.head || '', -1);
    }

    function saveDraft() {
        try {
            localStorage.setItem(DRAFT_KEY, JSON.stringify(readFormState()));
            if (draftBadge) {
                draftBadge.classList.remove('hidden');
                draftBadge.textContent = '草稿已保存 ' + new Date().toLocaleTimeString();
            }
        } catch (e) { /* ignore */ }
    }

    function scheduleDraftSave() {
        if (draftTimer) clearTimeout(draftTimer);
        draftTimer = setTimeout(saveDraft, DRAFT_DEBOUNCE_MS);
    }

    function loadDraft() {
        try {
            var raw = localStorage.getItem(DRAFT_KEY);
            if (!raw) return;
            var state = JSON.parse(raw);
            if (state && typeof state === 'object' && !isFormEmpty(state)) {
                writeFormState(state);
            }
        } catch (e) { /* ignore */ }
    }

    function clearDraft() {
        try { localStorage.removeItem(DRAFT_KEY); } catch (e) { /* ignore */ }
    }

    function showError(msg) {
        if (!errorEl) return;
        errorEl.textContent = msg;
        errorEl.classList.remove('hidden');
    }

    function hideError() {
        if (!errorEl) return;
        errorEl.classList.add('hidden');
    }

    function showUndoBar(show) {
        if (undoExampleBtn) undoExampleBtn.classList.toggle('hidden', !show);
        if (exampleHint) exampleHint.classList.toggle('hidden', !show);
    }

    function fillExample() {
        var current = readFormState();
        if (!isFormEmpty(current) &&
            !window.confirm('填入示例将覆盖当前内容，是否继续？\n\n仍可点击「撤回」恢复。'))
            return;
        snapshotBeforeExample = current;
        writeFormState(EXAMPLE);
        showUndoBar(true);
        hideError();
        scheduleDraftSave();
    }

    function undoExample() {
        if (!snapshotBeforeExample) return;
        writeFormState(snapshotBeforeExample);
        snapshotBeforeExample = null;
        showUndoBar(false);
        hideError();
        scheduleDraftSave();
    }

    function loadScript(src) {
        return new Promise(function (resolve, reject) {
            var s = document.createElement('script');
            s.src = src;
            s.onload = resolve;
            s.onerror = reject;
            document.head.appendChild(s);
        });
    }

    function ensureAce() {
        if (aceLoaded) return Promise.resolve();
        return loadScript(ACE_BASE + '/ace.js').then(function () {
            return Promise.all([
                loadScript(ACE_BASE + '/mode-c_cpp.js'),
                loadScript(ACE_BASE + '/theme-dracula.js')
            ]);
        }).then(function () {
            ace.config.set('basePath', ACE_BASE);
            aceLoaded = true;
        });
    }

    function initPreviewEditor() {
        if (previewEditor) return;
        previewEditor = ace.edit('previewEditor');
        previewEditor.setTheme('ace/theme/dracula');
        previewEditor.session.setMode('ace/mode/c_cpp');
        previewEditor.setReadOnly(true);
        previewEditor.setOptions({
            fontSize: '14px',
            showPrintMargin: false,
            highlightActiveLine: false,
            readOnly: true
        });
        previewEditor.renderer.setScrollMargin(8, 8, 0, 0);
        previewEditor.session.setUseWorker(false);
    }

    function openFullPreview() {
        var data = getFormData();
        if (!data.title && !data.desc && !data.head) {
            showError('请先填写一些内容再预览');
            return;
        }
        hideError();

        modalTitle.textContent = '预览. ' + (data.title || '未命名题目');
        modalMeta.innerHTML = buildMetaHtml(data.star, data.cpu_limit, data.mem_limit, currentAuthor);
        enhanceDescElement(modalDesc, data.desc || '（暂无描述）');

        previewModal.classList.remove('hidden');
        document.body.classList.add('modal-open');

        ensureAce().then(function () {
            initPreviewEditor();
            previewEditor.setValue(data.head || '', -1);
            previewEditor.clearSelection();
            previewEditor.resize();
        }).catch(function () {
            showError('编辑器加载失败，无法打开预览');
            closeFullPreview();
        });
    }

    function closeFullPreview() {
        previewModal.classList.add('hidden');
        document.body.classList.remove('modal-open');
        if (previewEditor) previewEditor.resize();
    }

    function requireLogin() {
        return fetch('/api/me', { credentials: 'same-origin' })
            .then(function (r) { return r.json(); })
            .then(function (data) {
                if (!data.data || (!data.data.username && !data.data.nickname)) {
                    window.location.href = '/login';
                    return false;
                }
                currentAuthor = data.data.nickname || data.data.username || '我';
                updatePreview();
                return true;
            })
            .catch(function () {
                window.location.href = '/login';
                return false;
            });
    }

    FIELD_NAMES.forEach(function (name) {
        var el = form.querySelector('[name="' + name + '"]');
        if (!el) return;
        el.addEventListener('input', function () {
            updatePreview();
            scheduleDraftSave();
        });
        el.addEventListener('change', function () {
            updatePreview();
            scheduleDraftSave();
        });
    });

    if (fillExampleBtn) fillExampleBtn.addEventListener('click', fillExample);
    if (undoExampleBtn) undoExampleBtn.addEventListener('click', undoExample);
    if (openFullPreviewBtn) openFullPreviewBtn.addEventListener('click', openFullPreview);
    if (closePreviewBtn) closePreviewBtn.addEventListener('click', closeFullPreview);
    if (previewBackdrop) previewBackdrop.addEventListener('click', closeFullPreview);

    document.addEventListener('keydown', function (e) {
        if (e.key === 'Escape' && previewModal && !previewModal.classList.contains('hidden'))
            closeFullPreview();
    });

    requireLogin().then(function (ok) {
        if (ok) loadDraft();
    });

    form.addEventListener('submit', async function (e) {
        e.preventDefault();
        hideError();

        var body = getFormData();
        body.cpu_limit = parseInt(body.cpu_limit, 10);
        body.mem_limit = parseInt(body.mem_limit, 10);

        if (!body.star) {
            showError('请选择难度');
            return;
        }

        submitBtn.disabled = true;
        submitBtn.textContent = '提交中…';

        try {
            var resp = await fetch('/api/submit_question', {
                method: 'POST',
                credentials: 'same-origin',
                headers: { 'Content-Type': 'application/json;charset=utf-8' },
                body: JSON.stringify(body)
            });
            var data = await resp.json();

            if (data.errcode !== 0) {
                if (data.errcode === 1) {
                    window.location.href = '/login';
                    return;
                }
                showError(data.errmsg || '提交失败');
                return;
            }

            clearDraft();
            snapshotBeforeExample = null;
            var number = data.data && data.data.number;
            window.location.href = number ? '/question/' + number : '/all_questions';
        } catch (err) {
            showError('网络错误，请稍后重试');
        } finally {
            submitBtn.disabled = false;
            submitBtn.textContent = '提交题目';
        }
    });
})();
