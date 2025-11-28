// FILE: /emall/js/main.js

let mainPageIsLoading = false;
let mainPageCurrentIdx = -1;

async function loadMoreProducts(container, searchParams = {}) {
    if (mainPageIsLoading) return;
    mainPageIsLoading = true;

    try {
        const body = { current_idx: mainPageCurrentIdx, ...searchParams };
        const data = await request('/emall/search_next_item', 'POST', body);
        
        if (data.next_idx !== -1) {
            mainPageCurrentIdx = data.next_idx;
            const productCard = await renderProductCard(mainPageCurrentIdx);
            if(productCard) container.appendChild(productCard);
            // 递归加载直到填满一页或没有更多
            if(container.children.length < 16 && data.next_idx !== -1) {
                mainPageIsLoading = false; // 允许下一次递归调用
                loadMoreProducts(container, searchParams);
            }
        } else {
             if(container.children.length === 0) container.innerHTML = "<p>没有找到任何商品。</p>";
             mainPageCurrentIdx = -1; // 表示没有更多了
        }
    } catch (error) {
        console.error('加载商品失败:', error);
    } finally {
        mainPageIsLoading = false;
    }
}

function renderMainPage() {
    mainPageCurrentIdx = -1; // 重置
    const app = document.getElementById('app');
    app.innerHTML = `
        <div class="container">
            <h2>热门商品</h2>
            <div id="product-grid" class="product-grid"></div>
        </div>
    `;

    const productGrid = document.getElementById('product-grid');
    loadMoreProducts(productGrid); // 初始加载

    window.onscroll = () => {
        if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - 100) {
            if (mainPageCurrentIdx !== -1) { // 确认还有更多商品
                loadMoreProducts(productGrid);
            }
        }
    };
}