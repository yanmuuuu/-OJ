(function() {
    var ACE_BASE = 'https://cdn.jsdelivr.net/npm/ace-builds@1.32.0/src-min-noconflict';

    var CPP_SNIPPETS = [
        { word: 'vec', insert: 'vector<>', meta: 'STL' },
        { word: 'vector', insert: 'vector<>', meta: 'STL' },
        { word: 'str', insert: 'string', meta: 'STL' },
        { word: 'string', insert: 'string', meta: 'STL' },
        { word: 'map', insert: 'map<, >', meta: 'STL' },
        { word: 'set', insert: 'set<>', meta: 'STL' },
        { word: 'unordered_map', insert: 'unordered_map<, >', meta: 'STL' },
        { word: 'unordered_set', insert: 'unordered_set<>', meta: 'STL' },
        { word: 'pair', insert: 'pair<, >', meta: 'STL' },
        { word: 'queue', insert: 'queue<>', meta: 'STL' },
        { word: 'stack', insert: 'stack<>', meta: 'STL' },
        { word: 'priority_queue', insert: 'priority_queue<>', meta: 'STL' },
        { word: 'deque', insert: 'deque<>', meta: 'STL' },
        { word: 'list', insert: 'list<>', meta: 'STL' },
        { word: 'sort', insert: 'sort(, )', meta: 'algorithm' },
        { word: 'reverse', insert: 'reverse(, )', meta: 'algorithm' },
        { word: 'max', insert: 'max(, )', meta: 'algorithm' },
        { word: 'min', insert: 'min(, )', meta: 'algorithm' },
        { word: 'swap', insert: 'swap(, )', meta: 'algorithm' },
        { word: 'lower_bound', insert: 'lower_bound(, )', meta: 'algorithm' },
        { word: 'upper_bound', insert: 'upper_bound(, )', meta: 'algorithm' },
        { word: 'binary_search', insert: 'binary_search(, , )', meta: 'algorithm' },
        { word: 'memset', insert: 'memset(, , )', meta: 'cstring' },
        { word: 'memcpy', insert: 'memcpy(, , )', meta: 'cstring' },
        { word: 'for', insert: 'for (int i = 0; i < n; i++) {\n    \n}', meta: 'snippet' },
        { word: 'while', insert: 'while () {\n    \n}', meta: 'snippet' },
        { word: 'if', insert: 'if () {\n    \n}', meta: 'snippet' },
        { word: 'else', insert: 'else {\n    \n}', meta: 'snippet' },
        { word: 'class', insert: 'class Solution {\npublic:\n    \n};', meta: 'snippet' },
        { word: 'struct', insert: 'struct Node {\n    \n};', meta: 'snippet' },
        { word: 'cout', insert: 'cout <<  << endl;', meta: 'iostream' },
        { word: 'cin', insert: 'cin >> ;', meta: 'iostream' },
        { word: 'return', insert: 'return ;', meta: 'keyword' },
        { word: 'nullptr', insert: 'nullptr', meta: 'keyword' },
        { word: 'auto', insert: 'auto ', meta: 'keyword' },
        { word: 'const', insert: 'const ', meta: 'keyword' },
        { word: 'static', insert: 'static ', meta: 'keyword' },
        { word: 'size_t', insert: 'size_t', meta: 'type' },
        { word: 'long long', insert: 'long long', meta: 'type' },
        { word: 'double', insert: 'double', meta: 'type' },
        { word: 'bool', insert: 'bool', meta: 'type' },
        { word: 'char', insert: 'char', meta: 'type' },
        { word: 'int', insert: 'int', meta: 'type' },
        { word: 'void', insert: 'void', meta: 'type' }
    ];

    var STATUS_MAP = {
        '-1': { text: '代码为空', cls: 'error' },
        '-2': { text: '判题服务暂时不可用', cls: 'error' },
        '-3': { text: '编译错误', cls: 'compile' },
        '6': { text: '超出内存限制', cls: 'error' },
        '8': { text: '浮点运算异常', cls: 'error' },
        '11': { text: '运行时错误', cls: 'error' },
        '24': { text: '超出时间限制', cls: 'error' }
    };

    var DEFAULT_RUN_CASE = [
        '#ifndef COMPILER_ONLINE',
        '#include "head.cpp"',
        '#endif',
        '',
        'void runCustom() {',
        '    Solution s;',
        '    // 在此修改测试数据',
        '}',
        '',
        'int main() {',
        '    runCustom();',
        '    return 0;',
        '}'
    ].join('\n');

    var solutionEditor = null;
    var caseEditor = null;
    var fontSize = 14;
    var cases = [];
    var activeCaseIndex = 0;
    var carouselIndex = 0;
    var questionNumber = '';
    var storageKey = '';

    function escapeHtml(s) {
        return String(s).replace(/[&<>"']/g, function(c) {
            return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' }[c];
        });
    }

    function loadScript(src) {
        return new Promise(function(resolve, reject) {
            var s = document.createElement('script');
            s.src = src;
            s.onload = resolve;
            s.onerror = reject;
            document.head.appendChild(s);
        });
    }

    function loadAceAssets() {
        return loadScript(ACE_BASE + '/ace.js').then(function() {
            return Promise.all([
                loadScript(ACE_BASE + '/ext-language_tools.js'),
                loadScript(ACE_BASE + '/mode-c_cpp.js'),
                loadScript(ACE_BASE + '/theme-dracula.js')
            ]);
        });
    }

    function getTemplate(id) {
        var el = document.getElementById(id);
        if (!el) return '';
        return (el.textContent || el.innerText || '').trim();
    }

    function enhanceProblemDesc() {
        var desc = document.getElementById('problemDesc');
        if (!desc) return;
        var raw = desc.textContent;
        var html = escapeHtml(raw);
        html = html.replace(/`([^`]+)`/g, '<span class="inline-code">$1</span>');
        html = html.replace(/\b(nums|target|vector|int|string)\b/g, '<span class="inline-code">$1</span>');
        desc.innerHTML = html;
    }

    function registerCppCompleter() {
        var langTools = ace.require('ace/ext/language_tools');
        langTools.addCompleter({
            getCompletions: function(editor, session, pos, prefix, callback) {
                if (!prefix || prefix.length < 1) {
                    callback(null, []);
                    return;
                }
                var lower = prefix.toLowerCase();
                var results = [];
                var seen = {};
                CPP_SNIPPETS.forEach(function(item) {
                    if (item.word.toLowerCase().indexOf(lower) !== 0) return;
                    if (seen[item.insert]) return;
                    seen[item.insert] = true;
                    results.push({
                        caption: item.word,
                        value: item.insert,
                        meta: item.meta,
                        score: item.word.length === lower.length ? 1000 : 100 - item.word.length
                    });
                });
                callback(null, results);
            }
        });
    }

    function createAceEditor(el, value) {
        var editor = ace.edit(el);
        editor.setTheme('ace/theme/dracula');
        editor.session.setMode('ace/mode/c_cpp');
        editor.setOptions({
            fontSize: fontSize + 'px',
            showPrintMargin: false,
            showGutter: true,
            highlightActiveLine: true,
            highlightSelectedWord: true,
            enableBasicAutocompletion: true,
            enableLiveAutocompletion: true,
            enableSnippets: false,
            tabSize: 4,
            useSoftTabs: true,
            wrap: false,
            scrollPastEnd: 0.2,
            animatedScroll: true
        });
        editor.renderer.setScrollMargin(6, 6, 0, 0);
        editor.session.setUseWorker(false);
        editor.setValue(value || '', -1);
        editor.clearSelection();
        return editor;
    }

    function resizeEditors() {
        if (solutionEditor) solutionEditor.resize();
        if (caseEditor) caseEditor.resize();
    }

    function initPanelResizer() {
        var resizer = document.getElementById('panelResizer');
        var panel = document.getElementById('panelLeft');
        var workspace = document.getElementById('workspace');
        if (!resizer || !panel || !workspace) return;

        var dragging = false;
        resizer.addEventListener('mousedown', function(e) {
            dragging = true;
            resizer.classList.add('dragging');
            document.body.style.cursor = 'col-resize';
            document.body.style.userSelect = 'none';
            e.preventDefault();
        });

        document.addEventListener('mousemove', function(e) {
            if (!dragging) return;
            var rect = workspace.getBoundingClientRect();
            var pct = ((e.clientX - rect.left) / rect.width) * 100;
            pct = Math.max(22, Math.min(58, pct));
            panel.style.width = pct + '%';
            resizeEditors();
        });

        document.addEventListener('mouseup', function() {
            if (!dragging) return;
            dragging = false;
            resizer.classList.remove('dragging');
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
        });
    }

    function initVerticalResizer() {
        var resizer = document.getElementById('panelVResizer');
        var codeZone = document.getElementById('solveCodeZone');
        var casePanel = document.getElementById('casePanel');
        var panelRight = codeZone && codeZone.parentElement;
        if (!resizer || !codeZone || !casePanel || !panelRight) return;

        var dragging = false;
        var startY = 0;
        var startCodeH = 0;

        resizer.addEventListener('mousedown', function(e) {
            if (casePanel.classList.contains('collapsed')) return;
            dragging = true;
            startY = e.clientY;
            startCodeH = codeZone.offsetHeight;
            resizer.classList.add('dragging');
            document.body.style.cursor = 'row-resize';
            document.body.style.userSelect = 'none';
            e.preventDefault();
        });

        document.addEventListener('mousemove', function(e) {
            if (!dragging) return;
            var delta = e.clientY - startY;
            var total = panelRight.offsetHeight - resizer.offsetHeight;
            var newCodeH = Math.max(120, Math.min(total - 160, startCodeH + delta));
            codeZone.style.flex = '0 0 ' + newCodeH + 'px';
            casePanel.style.flex = '1 1 0';
            resizeEditors();
        });

        document.addEventListener('mouseup', function() {
            if (!dragging) return;
            dragging = false;
            resizer.classList.remove('dragging');
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
        });
    }

    function initCasePanelCollapse() {
        var casePanel = document.getElementById('casePanel');
        var casePanelBody = document.getElementById('casePanelBody');
        var vResizer = document.getElementById('panelVResizer');
        var codeZone = document.getElementById('solveCodeZone');
        var collapseBtn = document.getElementById('caseCollapseBtn');
        var collapseIcon = document.getElementById('caseCollapseIcon');
        var collapseLabel = document.getElementById('caseCollapseLabel');
        if (!casePanel || !casePanelBody || !collapseBtn) return;

        var collapseKey = 'oj_case_collapsed_' + questionNumber;
        var savedCodeFlex = '';

        function applyCollapsed(collapsed) {
            casePanel.classList.toggle('collapsed', collapsed);
            if (vResizer) vResizer.classList.toggle('hidden', collapsed);
            if (collapsed) {
                if (codeZone) {
                    savedCodeFlex = codeZone.style.flex || '';
                    codeZone.style.flex = '1 1 auto';
                }
                if (collapseIcon) collapseIcon.textContent = '▴';
                if (collapseLabel) collapseLabel.textContent = '展开';
                collapseBtn.title = '展开测试区';
            } else {
                if (codeZone && savedCodeFlex) codeZone.style.flex = savedCodeFlex;
                if (collapseIcon) collapseIcon.textContent = '▾';
                if (collapseLabel) collapseLabel.textContent = '收起';
                collapseBtn.title = '收起测试区';
            }
            try {
                sessionStorage.setItem(collapseKey, collapsed ? '1' : '0');
            } catch (e) { /* ignore */ }
            setTimeout(resizeEditors, 60);
        }

        collapseBtn.addEventListener('click', function() {
            applyCollapsed(!casePanel.classList.contains('collapsed'));
        });

        try {
            if (sessionStorage.getItem(collapseKey) === '1') applyCollapsed(true);
        } catch (e) { /* ignore */ }
    }

    function saveCasesToStorage() {
        syncActiveCaseContent();
        try {
            localStorage.setItem(storageKey, JSON.stringify(cases));
        } catch (e) { /* ignore quota */ }
    }

    function loadCasesFromStorage() {
        var template = getTemplate('runCaseTemplate') || DEFAULT_RUN_CASE;
        try {
            var raw = localStorage.getItem(storageKey);
            if (raw) {
                var parsed = JSON.parse(raw);
                if (Array.isArray(parsed) && parsed.length > 0) {
                    cases = parsed.map(function(c, i) {
                        return { id: c.id || i + 1, content: c.content || template };
                    });
                    return;
                }
            }
        } catch (e) { /* fall through */ }
        cases = [{ id: 1, content: template }];
    }

    function syncActiveCaseContent() {
        if (!caseEditor || !cases[activeCaseIndex]) return;
        cases[activeCaseIndex].content = caseEditor.getValue();
    }

    function renderCaseTabs() {
        var container = document.getElementById('caseTabs');
        if (!container) return;
        container.innerHTML = '';
        cases.forEach(function(c, idx) {
            var btn = document.createElement('button');
            btn.type = 'button';
            btn.className = 'case-tab' + (idx === activeCaseIndex ? ' active' : '');
            btn.textContent = 'Case ' + (idx + 1);
            btn.dataset.index = String(idx);
            btn.addEventListener('click', function() {
                selectCase(idx);
            });
            container.appendChild(btn);
        });
        scrollActiveTabIntoView();
    }

    function scrollActiveTabIntoView() {
        var container = document.getElementById('caseTabs');
        var active = container && container.querySelector('.case-tab.active');
        if (active && active.scrollIntoView) {
            active.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
        }
    }

    function selectCase(idx) {
        if (idx < 0 || idx >= cases.length) return;
        syncActiveCaseContent();
        activeCaseIndex = idx;
        caseEditor.setValue(cases[idx].content, -1);
        caseEditor.clearSelection();
        renderCaseTabs();
        saveCasesToStorage();
    }

    function addCase() {
        syncActiveCaseContent();
        var template = getTemplate('runCaseTemplate') || DEFAULT_RUN_CASE;
        var nextId = cases.reduce(function(max, c) { return Math.max(max, c.id); }, 0) + 1;
        cases.push({ id: nextId, content: template });
        selectCase(cases.length - 1);
        saveCasesToStorage();
    }

    function navigateCaseTab(delta) {
        selectCase((activeCaseIndex + delta + cases.length) % cases.length);
    }

    function setCarouselIndex(index) {
        carouselIndex = Math.max(0, Math.min(1, index));
        var track = document.getElementById('carouselTrack');
        if (track) track.style.transform = 'translateX(-' + (carouselIndex * 100) + '%)';
        document.querySelectorAll('.carousel-dot').forEach(function(dot) {
            dot.classList.toggle('active', parseInt(dot.dataset.index, 10) === carouselIndex);
        });
        var prev = document.getElementById('carouselPrev');
        var next = document.getElementById('carouselNext');
        if (prev) prev.disabled = carouselIndex === 0;
        if (next) next.disabled = carouselIndex === 1;
    }

    function initCarousel() {
        var prev = document.getElementById('carouselPrev');
        var next = document.getElementById('carouselNext');
        if (prev) prev.addEventListener('click', function() { setCarouselIndex(carouselIndex - 1); });
        if (next) next.addEventListener('click', function() { setCarouselIndex(carouselIndex + 1); });
        document.querySelectorAll('.carousel-dot').forEach(function(dot) {
            dot.addEventListener('click', function() {
                setCarouselIndex(parseInt(dot.dataset.index, 10));
            });
        });

        var viewport = document.querySelector('.carousel-viewport');
        if (!viewport) return;
        var touchStartX = 0;
        viewport.addEventListener('touchstart', function(e) {
            touchStartX = e.changedTouches[0].screenX;
        }, { passive: true });
        viewport.addEventListener('touchend', function(e) {
            var dx = e.changedTouches[0].screenX - touchStartX;
            if (Math.abs(dx) > 50) setCarouselIndex(carouselIndex + (dx < 0 ? 1 : -1));
        }, { passive: true });
    }

    function parseTests(stdout) {
        if (!stdout) return [];
        return stdout.trim().split('\n').filter(Boolean).map(function(line) {
            var passed = /passed\s*$/i.test(line) && !/not\s+passed/i.test(line);
            return { line: line, passed: passed };
        });
    }

    function getErrorVerdict(data) {
        var info = STATUS_MAP[String(data.status)];
        return {
            text: data.reason || (info ? info.text : '未通过'),
            cls: info ? info.cls : 'error'
        };
    }

    function buildSubmitVerdict(stdout) {
        var tests = parseTests(stdout);
        if (tests.length === 0) return { text: '运行成功', cls: 'success' };
        var passCount = tests.filter(function(t) { return t.passed; }).length;
        if (passCount === tests.length) {
            return { text: '全部通过 · ' + passCount + '/' + tests.length, cls: 'success' };
        }
        if (passCount === 0) {
            return { text: '未通过 · 0/' + tests.length, cls: 'error' };
        }
        return { text: '部分通过 · ' + passCount + '/' + tests.length, cls: 'warning' };
    }

    function renderRunResult(data) {
        if (data.status === 0) {
            var out = data.stdout || '';
            var err = data.stderr || '';
            var html = '';
            if (out) html += '<div class="result-pre result-pre-stdout">' + escapeHtml(out) + '</div>';
            if (err) html += '<div class="result-pre result-pre-stderr">' + escapeHtml(err) + '</div>';
            if (!html) html = '<p class="result-placeholder">程序运行成功，无输出</p>';
            return { title: '运行结果', badge: '成功', cls: 'success', html: html };
        }
        if (data.status === -3) {
            return {
                title: '运行结果',
                badge: '编译错误',
                cls: 'compile',
                html: '<div class="result-pre">' + escapeHtml(data.reason || '编译失败') + '</div>'
            };
        }
        var v = getErrorVerdict(data);
        return {
            title: '运行结果',
            badge: v.text,
            cls: v.cls,
            html: '<p class="result-error-msg">' + escapeHtml(v.text) + '</p>'
        };
    }

    function renderSubmitResult(data) {
        if (data.status === 0) {
            var tests = parseTests(data.stdout);
            var verdict = buildSubmitVerdict(data.stdout);
            var html = '';
            if (tests.length > 0) {
                html += '<div class="test-grid">';
                tests.forEach(function(t) {
                    html += '<div class="test-item ' + (t.passed ? 'pass' : 'fail') + '">';
                    html += '<span class="test-dot"></span>';
                    html += escapeHtml(t.line);
                    html += '</div>';
                });
                html += '</div>';
            } else if (data.stdout) {
                html += '<div class="result-pre">' + escapeHtml(data.stdout) + '</div>';
            }
            return { title: '提交结果', badge: verdict.text, cls: verdict.cls, html: html };
        }
        if (data.status === -3) {
            return {
                title: '提交结果',
                badge: '编译错误',
                cls: 'compile',
                html: '<div class="result-pre">' + escapeHtml(data.reason || '编译失败') + '</div>'
            };
        }
        var v = getErrorVerdict(data);
        return {
            title: '提交结果',
            badge: v.text,
            cls: v.cls,
            html: '<p class="result-error-msg">' + escapeHtml(v.text) + '</p>'
        };
    }

    function showResultView(payload) {
        var titleEl = document.getElementById('resultViewTitle');
        var badgeEl = document.getElementById('resultViewBadge');
        var bodyEl = document.getElementById('resultViewBody');
        var view = document.getElementById('resultView');
        if (!titleEl || !badgeEl || !bodyEl || !view) return;

        titleEl.textContent = payload.title;
        badgeEl.textContent = payload.badge;
        badgeEl.className = 'result-view-badge ' + payload.cls;
        view.className = 'result-view result-view-' + payload.cls;
        bodyEl.innerHTML = payload.html;
        setCarouselIndex(1);
    }

    function setBtnLoading(btn, on, idleText) {
        if (!btn) return;
        btn.disabled = on;
        btn.classList.toggle('loading', on);
        btn.textContent = on ? '' : idleText;
    }

    function fetchJson(url, body) {
        return fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        }).then(function(res) {
            return res.text().then(function(text) {
                if (!text) throw new Error('SERVICE_UNAVAILABLE');
                try { return JSON.parse(text); }
                catch (e) { throw new Error('SERVICE_UNAVAILABLE'); }
            });
        });
    }

    function initEditorsAndActions() {
        ace.config.set('basePath', ACE_BASE);
        registerCppCompleter();

        var codeTemplate = getTemplate('codeTemplate');
        solutionEditor = createAceEditor('editor', codeTemplate);
        solutionEditor.focus();
        solutionEditor.gotoLine(1, 0, true);

        loadCasesFromStorage();
        caseEditor = createAceEditor('caseEditor', cases[activeCaseIndex].content);
        renderCaseTabs();

        caseEditor.session.on('change', function() {
            saveCasesToStorage();
        });

        document.getElementById('fontInc').addEventListener('click', function() {
            fontSize = Math.min(22, fontSize + 1);
            solutionEditor.setFontSize(fontSize);
            caseEditor.setFontSize(fontSize);
        });
        document.getElementById('fontDec').addEventListener('click', function() {
            fontSize = Math.max(11, fontSize - 1);
            solutionEditor.setFontSize(fontSize);
            caseEditor.setFontSize(fontSize);
        });

        window.addEventListener('resize', resizeEditors);

        var runBtn = document.getElementById('runBtn');
        var submitBtn = document.getElementById('submitBtn');

        document.getElementById('caseTabAdd').addEventListener('click', addCase);
        document.getElementById('caseTabPrev').addEventListener('click', function() { navigateCaseTab(-1); });
        document.getElementById('caseTabNext').addEventListener('click', function() { navigateCaseTab(1); });

        runBtn.addEventListener('click', function() {
            if (runBtn.disabled) return;
            syncActiveCaseContent();
            setBtnLoading(runBtn, true, '运行');
            fetchJson('/run/' + questionNumber, {
                input: '',
                code: solutionEditor.getValue(),
                case_code: cases[activeCaseIndex].content
            })
            .then(function(data) {
                setBtnLoading(runBtn, false, '运行');
                showResultView(renderRunResult(data));
            })
            .catch(function() {
                setBtnLoading(runBtn, false, '运行');
                showResultView({
                    title: '运行结果',
                    badge: '失败',
                    cls: 'error',
                    html: '<p class="result-error-msg">判题服务暂时不可用，请稍后重试</p>'
                });
            });
        });

        submitBtn.addEventListener('click', function() {
            if (submitBtn.disabled) return;
            setBtnLoading(submitBtn, true, '提交判题');
            fetchJson('/judge/' + questionNumber, {
                input: '',
                code: solutionEditor.getValue()
            })
            .then(function(data) {
                setBtnLoading(submitBtn, false, '提交判题');
                showResultView(renderSubmitResult(data));
            })
            .catch(function() {
                setBtnLoading(submitBtn, false, '提交判题');
                showResultView({
                    title: '提交结果',
                    badge: '失败',
                    cls: 'error',
                    html: '<p class="result-error-msg">判题服务暂时不可用，请稍后重试</p>'
                });
            });
        });

        solutionEditor.commands.addCommand({
            name: 'runCode',
            bindKey: { win: 'Ctrl-Enter', mac: 'Command-Enter' },
            exec: function() { runBtn.click(); }
        });
        solutionEditor.commands.addCommand({
            name: 'submitCode',
            bindKey: { win: 'Ctrl-Shift-Enter', mac: 'Command-Shift-Enter' },
            exec: function() { submitBtn.click(); }
        });
    }

    document.addEventListener('DOMContentLoaded', function() {
        questionNumber = (document.getElementById('questionNumber').textContent || '').trim();
        storageKey = 'oj_run_cases_' + questionNumber;

        enhanceProblemDesc();
        initPanelResizer();
        initVerticalResizer();
        initCasePanelCollapse();
        initCarousel();
        setCarouselIndex(0);

        loadAceAssets()
            .then(initEditorsAndActions)
            .catch(function() {
                var el = document.getElementById('editor');
                if (el) {
                    el.innerHTML = '<p style="padding:24px;color:#7a92ad;">编辑器加载失败，请刷新页面重试</p>';
                }
            });
    });
})();
