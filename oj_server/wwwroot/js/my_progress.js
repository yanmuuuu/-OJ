document.addEventListener('DOMContentLoaded', function() {
    var summaryEl = document.getElementById('progressSummary');
    var listEl = document.getElementById('progressList');
    var emptyEl = document.getElementById('progressEmpty');
    var filterStatus = document.getElementById('filterStatus');
    var filterStar = document.getElementById('filterStar');
    var allItems = [];

    function escapeHtml(s) {
        var d = document.createElement('div');
        d.textContent = s;
        return d.innerHTML;
    }

    function statusLabel(status) {
        return status === 'solved' ? '已解答' : '尝试过';
    }

    function renderList(items) {
        listEl.innerHTML = '';
        if (!items || items.length === 0) {
            emptyEl.classList.remove('hidden');
            return;
        }
        emptyEl.classList.add('hidden');

        items.forEach(function(item) {
            var status = item._status || 'attempted';
            var article = document.createElement('article');
            article.className = 'question-card';
            article.dataset.star = item.star || '';
            article.innerHTML =
                '<a href="/question/' + escapeHtml(item.number) + '">' +
                    '<div class="card-number">' + escapeHtml(item.number) + '</div>' +
                    '<h2 class="card-title">' + escapeHtml(item.title || '') + '</h2>' +
                    '<span class="difficulty ' + escapeHtml(item.star || '') + '">' + escapeHtml(item.star || '') + '</span>' +
                    '<span class="card-updated">' +
                        '<span class="card-status card-status-' + escapeHtml(status) + '">' + escapeHtml(statusLabel(status)) + '</span> ' +
                        escapeHtml(item.updated_at || '') +
                    '</span>' +
                '</a>';
            listEl.appendChild(article);
        });
    }

    function applyFilters() {
        var statusVal = filterStatus ? filterStatus.value : 'all';
        var starVal = filterStar ? filterStar.value : 'all';
        var filtered = allItems.filter(function(item) {
            var matchStatus = statusVal === 'all' || item._status === statusVal;
            var matchStar = starVal === 'all' || item.star === starVal;
            return matchStatus && matchStar;
        });
        renderList(filtered);
    }

    function updateSummary(stats) {
        stats = stats || { solved: 0, attempted: 0 };
        summaryEl.textContent = '已解答 ' + (stats.solved || 0) + ' 题 · 尝试过 ' + (stats.attempted || 0) + ' 题';
    }

    if (filterStatus) filterStatus.addEventListener('change', applyFilters);
    if (filterStar) filterStar.addEventListener('change', applyFilters);

    fetch('/api/my_progress', { credentials: 'same-origin' })
        .then(function(r) { return r.json(); })
        .then(function(data) {
            if (data.errcode !== 0) {
                window.location.href = '/login';
                return;
            }
            var payload = data.data || {};
            var solved = (payload.solved || []).map(function(x) {
                x._status = 'solved';
                return x;
            });
            var attempted = (payload.attempted || []).map(function(x) {
                x._status = 'attempted';
                return x;
            });
            allItems = solved.concat(attempted);
            updateSummary(payload.stats);
            applyFilters();
        })
        .catch(function() {
            summaryEl.textContent = '加载失败';
        });
});
