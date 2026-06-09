document.addEventListener('DOMContentLoaded', function() {
    var searchInput = document.getElementById('searchInput');
    var cards = document.querySelectorAll('.question-card');
    var emptyState = document.getElementById('emptyState');
    var totalCount = document.getElementById('totalCount');

    if (totalCount) {
        totalCount.textContent = cards.length;
    }

    if (!searchInput) return;

    function filter() {
        var term = searchInput.value.trim().toLowerCase();
        var visible = 0;
        cards.forEach(function(card) {
            var title = (card.dataset.title || '').toLowerCase();
            var number = (card.dataset.number || '').toLowerCase();
            var match = !term || title.indexOf(term) !== -1 || number.indexOf(term) !== -1;
            card.style.display = match ? '' : 'none';
            if (match) visible++;
        });
        if (emptyState) {
            emptyState.classList.toggle('hidden', visible > 0);
        }
    }

    searchInput.addEventListener('input', filter);

    searchInput.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') {
            searchInput.value = '';
            filter();
            searchInput.blur();
        }
    });
});
