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
        '-1': { text: '代码为空', cls: 'error', showDetail: false },
        '-2': { text: '判题服务暂时不可用', cls: 'error', showDetail: false },
        '-3': { text: '编译错误', cls: 'compile', showDetail: true },
        '6': { text: '超出内存限制', cls: 'error', showDetail: false },
        '8': { text: '浮点运算异常', cls: 'error', showDetail: false },
        '11': { text: '运行时错误', cls: 'error', showDetail: false },
        '24': { text: '超出时间限制', cls: 'error', showDetail: false }
    };

    var editorInstance = null;
    var fontSize = 14;

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

    function getCodeTemplate() {
        var el = document.getElementById('codeTemplate');
        if (!el) return '';
        return el.textContent || el.innerText || '';
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
            if (editorInstance) editorInstance.resize();
        });

        document.addEventListener('mouseup', function() {
            if (!dragging) return;
            dragging = false;
            resizer.classList.remove('dragging');
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
        });
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
            text: data.reason || (info ? info.text : '判题未通过'),
            cls: info ? info.cls : 'error'
        };
    }

    function buildSuccessVerdict(stdout) {
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

    function renderResult(data) {
        var html = '';
        if (data.status === 0) {
            var tests = parseTests(data.stdout);
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
        } else if (data.status === -3 && data.reason) {
            html += '<div class="result-pre">' + escapeHtml(data.reason) + '</div>';
        }
        return html;
    }

    function initEditor() {
        ace.config.set('basePath', ACE_BASE);
        registerCppCompleter();

        var editor = ace.edit('editor');
        editorInstance = editor;

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
            scrollPastEnd: 0.3,
            animatedScroll: true
        });
        editor.renderer.setScrollMargin(8, 8, 0, 0);
        editor.session.setUseWorker(false);

        var template = getCodeTemplate();
        editor.setValue(template, -1);
        editor.clearSelection();
        editor.focus();
        editor.gotoLine(1, 0, true);

        document.getElementById('fontInc').addEventListener('click', function() {
            fontSize = Math.min(22, fontSize + 1);
            editor.setFontSize(fontSize);
        });
        document.getElementById('fontDec').addEventListener('click', function() {
            fontSize = Math.max(11, fontSize - 1);
            editor.setFontSize(fontSize);
        });

        window.addEventListener('resize', function() { editor.resize(); });

        var questionNumber = document.getElementById('questionNumber').textContent.trim();
        var submitBtn = document.getElementById('submitBtn');
        var resultPanel = document.getElementById('resultPanel');
        var resultStatus = document.getElementById('resultStatus');
        var resultBody = document.getElementById('resultBody');
        var resultHeader = document.getElementById('resultHeader');
        var collapsed = false;

        resultHeader.addEventListener('click', function() {
            collapsed = !collapsed;
            resultPanel.classList.toggle('collapsed', collapsed);
            document.getElementById('resultToggle').textContent = collapsed ? '展开' : '收起';
        });

        function setLoading(on) {
            submitBtn.disabled = on;
            submitBtn.classList.toggle('loading', on);
            submitBtn.textContent = on ? '' : '提交判题';
        }

        function showResult(verdict, html) {
            resultStatus.textContent = verdict.text;
            resultStatus.className = 'result-status ' + verdict.cls;
            resultBody.innerHTML = html;
            resultPanel.classList.remove('hidden', 'collapsed');
            collapsed = false;
            document.getElementById('resultToggle').textContent = '收起';
        }

        function submit() {
            if (submitBtn.disabled) return;
            setLoading(true);

            fetch('/judge/' + questionNumber, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ input: '', code: editor.getValue() })
            })
            .then(function(res) {
                return res.text().then(function(text) {
                    if (!text) throw new Error('SERVICE_UNAVAILABLE');
                    try { return JSON.parse(text); }
                    catch (e) { throw new Error('SERVICE_UNAVAILABLE'); }
                });
            })
            .then(function(data) {
                setLoading(false);
                var verdict = data.status === 0
                    ? buildSuccessVerdict(data.stdout)
                    : getErrorVerdict(data);
                showResult(verdict, renderResult(data));
            })
            .catch(function() {
                setLoading(false);
                showResult(
                    { text: '提交失败', cls: 'error' },
                    '<p style="color:var(--text-secondary);font-size:13px;">判题服务暂时不可用，请稍后重试</p>'
                );
            });
        }

        submitBtn.addEventListener('click', submit);
        editor.commands.addCommand({
            name: 'submitCode',
            bindKey: { win: 'Ctrl-Enter', mac: 'Command-Enter' },
            exec: submit
        });
    }

    document.addEventListener('DOMContentLoaded', function() {
        enhanceProblemDesc();
        initPanelResizer();
        loadAceAssets()
            .then(initEditor)
            .catch(function() {
                var el = document.getElementById('editor');
                if (el) {
                    el.innerHTML = '<p style="padding:24px;color:#7a92ad;">编辑器加载失败，请刷新页面重试</p>';
                }
            });
    });
})();
