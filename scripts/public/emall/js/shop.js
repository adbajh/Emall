// FILE: /emall/js/shop.js

let shopPageIsLoading = false;
let shopPageCurrentIdx = -1;

async function loadShopProducts(container, shopIdx) {
    if (shopPageIsLoading) return;
    shopPageIsLoading = true;

    try {
        const body = { current_idx: shopPageCurrentIdx, shop_idx: shopIdx };
        const data = await request('/emall/search_next_item', 'POST', body);
        
        if (data.next_idx !== -1) {
            shopPageCurrentIdx = data.next_idx;
            const productCard = await renderProductCard(shopPageCurrentIdx);
            if(productCard) container.appendChild(productCard);
        } else {
            if(container.children.length === 0) container.innerHTML = "<p>该店铺暂无商品。</p>";
            shopPageCurrentIdx = -1;
        }
    } catch (error) {
        console.error('加载店铺商品失败:', error);
    } finally {
        shopPageIsLoading = false;
    }
}

async function renderShopPage() {
    shopPageCurrentIdx = -1; // 重置
    const params = getQueryParams();
    const shopIdx = params.idx;
    if (!shopIdx) {
        document.getElementById('app').innerHTML = '<h1>无效的店铺 ID</h1>';
        return;
    }

    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const shopInfo = await request(`/emall/require_safe?info=shop&idx=${shopIdx}`);
        if (!shopInfo.result) throw new Error('店铺信息获取失败');

        app.innerHTML = `
            <div class="container">
                <h2>${shopInfo.name}</h2>
                <img src="${API_BASE_URL}/emall/require_image?info=shop&idx=${shopIdx}" alt="${shopInfo.name}" style="max-width: 200px; border-radius: 8px;">
                <p><strong>经理:</strong> <a href="/emall/user?idx=${shopInfo.manager_idx}" data-link>${shopInfo.manager}</a></p>
                <p><strong>运营状态:</strong> ${shopInfo.state === 0 ? '打烊' : '运营中'}</p>
                <p><strong>创建日期:</strong> ${shopInfo.date}</p>
                <p><strong>描述:</strong> ${shopInfo.description}</p>
                <button class="btn" onclick="navigateTo('/emall/messages?target_idx=${shopInfo.manager_idx}')">联系经理</button>
            </div>
            <div class="container">
                <h3>店铺商品</h3>
                <div id="shop-product-grid" class="product-grid"></div>
            </div>
        `;
        
        const productGrid = document.getElementById('shop-product-grid');
        loadShopProducts(productGrid, parseInt(shopIdx));

        window.onscroll = () => {
            if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - 100) {
                if (shopPageCurrentIdx !== -1) {
                    loadShopProducts(productGrid, parseInt(shopIdx));
                }
            }
        };

    } catch (error) {
        app.innerHTML = `<div class="container"><h2>店铺加载失败</h2><p>${error.message}</p></div>`;
    }
}