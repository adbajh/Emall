// FILE: /emall/js/search.js

let searchPageIsLoading = false;
let searchPageCurrentIdx = -1;
let currentSearchParams = {};

async function loadSearchResults(container) {
    if (searchPageIsLoading) return;
    searchPageIsLoading = true;

    try {
        const body = { current_idx: searchPageCurrentIdx, ...currentSearchParams };
        const data = await request('/emall/search_next_item', 'POST', body);
        
        if (data.next_idx !== -1) {
            searchPageCurrentIdx = data.next_idx;
            const productCard = await renderProductCard(searchPageCurrentIdx);
            if(productCard) container.appendChild(productCard);
        } else {
            if(container.children.length === 0) container.innerHTML = "<p>没有找到符合条件的商品。</p>";
            searchPageCurrentIdx = -1; // No more results
        }
    } catch (error) {
        console.error('加载搜索结果失败:', error);
    } finally {
        searchPageIsLoading = false;
    }
}

async function renderSearchPage() {
    const app = document.getElementById('app');
    app.innerHTML = `
        <div class="container">
            <h2>商品搜索</h2>
            <form id="search-form">
                <div class="form-group">
                    <label>关键字</label>
                    <input type="text" id="keywords" placeholder="输入商品关键字，用空格隔开">
                </div>
                <div class="form-group">
                    <label>商品类别</label>
                    <div id="type-buttons">加载中...</div>
                </div>
                <div class="form-group">
                    <label>店铺名称</label>
                    <input type="text" id="shop-name">
                </div>
                <div class="form-group">
                    <label>价格区间</label>
                    <input type="number" id="min-price" placeholder="最低价格" style="width: 45%;">
                    -
                    <input type="number" id="max-price" placeholder="最高价格" style="width: 45%;">
                </div>
                <div class="form-group">
                    <label>日期区间 (YYYY-MM-DD)</label>
                    <input type="date" id="min-date" style="width: 45%;">
                    -
                    <input type="date" id="max-date" style="width: 45%;">
                </div>
                <button type="submit" class="btn">搜索</button>
            </form>
            <div id="search-results-grid" class="product-grid"></div>
        </div>
    `;

    // 加载商品类别
    try {
        const typeData = await request('/emall/require_safe?info=type&idx=0');
        const typeButtonsContainer = document.getElementById('type-buttons');
        typeButtonsContainer.innerHTML = '';
        typeData.types.forEach(type => {
            const button = document.createElement('button');
            button.type = 'button';
            button.textContent = type;
            button.style = 'margin: 5px; cursor: pointer; border: 1px solid #ccc; background-color: #f0f0f0;';
            button.onclick = () => {
                button.classList.toggle('selected');
                button.style.backgroundColor = button.classList.contains('selected') ? '#007bff' : '#f0f0f0';
                button.style.color = button.classList.contains('selected') ? 'white' : 'black';
            };
            typeButtonsContainer.appendChild(button);
        });
    } catch (error) {
        document.getElementById('type-buttons').innerHTML = '类别加载失败';
    }

    // 搜索表单提交
    document.getElementById('search-form').addEventListener('submit', (e) => {
        e.preventDefault();
        const keywords = document.getElementById('keywords').value.split(' ').filter(k => k);
        const selectedTypes = Array.from(document.querySelectorAll('#type-buttons .selected')).map(b => b.textContent);
        
        currentSearchParams = {};
        keywords.forEach((key, i) => currentSearchParams[`key${i}`] = key);
        selectedTypes.forEach((type, i) => currentSearchParams[`type${i}`] = type);
        
        const shop = document.getElementById('shop-name').value;
        if(shop) currentSearchParams.shop = shop;

        const min_price = document.getElementById('min-price').value;
        if(min_price) currentSearchParams.min_price = parseFloat(min_price);

        const max_price = document.getElementById('max-price').value;
        if(max_price) currentSearchParams.max_price = parseFloat(max_price);
        
        const min_date = document.getElementById('min-date').value;
        if(min_date) currentSearchParams.min_date = min_date;

        const max_date = document.getElementById('max-date').value;
        if(max_date) currentSearchParams.max_date = max_date;

        // 重置并开始搜索
        searchPageCurrentIdx = -1;
        const resultsGrid = document.getElementById('search-results-grid');
        resultsGrid.innerHTML = '';
        loadSearchResults(resultsGrid);
    });
    
    window.onscroll = () => {
        if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - 100) {
            if (searchPageCurrentIdx !== -1) { // 确认还有更多商品
                loadSearchResults(document.getElementById('search-results-grid'));
            }
        }
    };
}