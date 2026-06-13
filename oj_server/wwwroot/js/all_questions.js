document.addEventListener('DOMContentLoaded', function() {
    var searchInput = document.getElementById('searchInput');
    var cards = document.querySelectorAll('.question-card');
    var emptyState = document.getElementById('emptyState');
    var totalCount = document.getElementById('totalCount');
    var filterStatus = document.getElementById('filterStatus');
    var filterStar = document.getElementById('filterStar');
    var progressMap = {};

    if (totalCount) {
        totalCount.textContent = cards.length;
    }

    function applyProgressBadges(map) {
        progressMap = map || {};
        cards.forEach(function(card) {
            var number = card.dataset.number;
            var status = progressMap[number];
            var wrap = card.querySelector('.card-status-wrap');
            if (!wrap) return;
            wrap.innerHTML = '';
            if (status === 'solved') {
                var badge = document.createElement('span');
                badge.className = 'card-status card-status-solved';
                badge.textContent = '已解答';
                wrap.appendChild(badge);
            } else if (status === 'attempted') {
                var badge2 = document.createElement('span');
                badge2.className = 'card-status card-status-attempted';
                badge2.textContent = '尝试过';
                wrap.appendChild(badge2);
            }
        });
        filter();
    }

    fetch('/api/my_progress', { credentials: 'same-origin' })
        .then(function(r) { return r.json(); })
        .then(function(data) {
            if (data.errcode === 0 && data.data && data.data.map) {
                applyProgressBadges(data.data.map);
            } else {
                applyProgressBadges({});
            }
        })
        .catch(function() {
            applyProgressBadges({});
        });

    function filter() {
        var term = searchInput ? searchInput.value.trim().toLowerCase() : '';
        var statusVal = filterStatus ? filterStatus.value : 'all';
        var starVal = filterStar ? filterStar.value : 'all';
        var visible = 0;

        cards.forEach(function(card) {
            var title = (card.dataset.title || '').toLowerCase();
            var number = (card.dataset.number || '').toLowerCase();
            var star = card.dataset.star || '';
            var author = (card.dataset.author || '').toLowerCase();
            var progress = progressMap[card.dataset.number] || 'none';

            var matchText = !term || title.indexOf(term) !== -1 || number.indexOf(term) !== -1 || author.indexOf(term) !== -1;
            var matchStatus = statusVal === 'all'
                || (statusVal === 'none' && !progressMap[card.dataset.number])
                || progress === statusVal;
            var matchStar = starVal === 'all' || star === starVal;
            var match = matchText && matchStatus && matchStar;

            card.style.display = match ? '' : 'none';
            if (match) visible++;
        });

        if (emptyState) {
            emptyState.classList.toggle('hidden', visible > 0);
        }
    }

    if (searchInput) {
        searchInput.addEventListener('input', filter);
        searchInput.addEventListener('keydown', function(e) {
            if (e.key === 'Escape') {
                searchInput.value = '';
                filter();
                searchInput.blur();
            }
        });
    }

    if (filterStatus) filterStatus.addEventListener('change', filter);
    if (filterStar) filterStar.addEventListener('change', filter);
});
