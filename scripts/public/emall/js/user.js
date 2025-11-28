// FILE: /emall/js/user.js

let userPageIsLoading = false;
let userPageCurrentIdx = -1;

async function loadUserProducts(container, sellerIdx) {
    if (userPageIsLoading) return;
    userPageIsLoading = true;

    try {
        const body = { current_idx: userPageCurrentIdx, seller_idx: sellerIdx };
        const data = await request('/emall/search_next_item', 'POST', body);
        
        if (data.next_idx !== -1) {
            userPageCurrentIdx = data.next_idx;
            const productCard = await renderProductCard(userPageCurrentIdx);
            if(productCard) container.appendChild(productCard);
        } else {
            if(container.children.length === 0) container.innerHTML = "<p>该用户暂未发布商品。</p>";
            userPageCurrentIdx = -1;
        }
    } catch (error) {
        console.error('加载用户商品失败:', error);
    } finally {
        userPageIsLoading = false;
    }
}

async function renderUserPage() {
    userPageCurrentIdx = -1; // 重置
    const params = getQueryParams();
    const userIdx = params.idx;
    if (!userIdx) {
        document.getElementById('app').innerHTML = '<h1>无效的用户 ID</h1>';
        return;
    }

    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const userInfo = await request(`/emall/require_safe?info=user&idx=${userIdx}`);
        if (!userInfo.result) throw new Error('用户信息获取失败');

        app.innerHTML = `
            <div class="container">
                <h2>${userInfo.name}的个人主页</h2>
                <img src="${API_BASE_URL}/emall/require_image?info=user&idx=${userIdx}" alt="${userInfo.name}" style="width: 150px; height: 150px; border-radius: 50%;">
                <p><strong>账号:</strong> ${userInfo.account}</p>
                <p><strong>电话:</strong> ${userInfo.phone || '未设置'}</p>
                <p><strong>邮箱:</strong> ${userInfo.email || '未设置'}</p>
                <p><strong>描述:</strong> ${userInfo.description || '暂无描述'}</p>
                <button class="btn" onclick="navigateTo('/emall/messages?target_idx=${userIdx}')">联系ta</button>
            </div>
            <div class="container">
                <h3>ta发布的商品</h3>
                <div id="user-product-grid" class="product-grid"></div>
            </div>
        `;

        const productGrid = document.getElementById('user-product-grid');
        loadUserProducts(productGrid, parseInt(userIdx));

        window.onscroll = () => {
            if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight - 100) {
                if (userPageCurrentIdx !== -1) {
                    loadUserProducts(productGrid, parseInt(userIdx));
                }
            }
        };

    } catch (error) {
        app.innerHTML = `<div class="container"><h2>用户加载失败</h2><p>${error.message}</p></div>`;
    }
}