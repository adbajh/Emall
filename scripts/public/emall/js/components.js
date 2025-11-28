// FILE: /emall/js/components.js

async function renderMenuBar() {
    const menuContainer = document.getElementById('menu-bar-container');
    const lastChatTarget = sessionStorage.getItem('last_chat_target');
    const messagesUrl = lastChatTarget ? `/emall/messages?target_idx=${lastChatTarget}` : '/emall/messages';
    let menuHTML = `
        <nav id="menu-bar">
            <a href="/emall" data-link>主页</a>
            <a href="/emall/search" data-link>搜索</a>
            <div class="right-menu">
    `;

    try {
        const data = await request('/emall/get_user_idx');
        if (data.user_idx !== -1) {
            // 已登录
            menuHTML += `
                <a href="/emall/manage" data-link>我的商店</a>
                <a href="/emall/orders" data-link>我的订单</a>
                <a href="${messagesUrl}" data-link>站内信</a>
                <a href="/emall/setting" data-link>设置</a>
            `;
        } else {
            // 未登录
            menuHTML += `
                <a href="/emall/login">登录</a>
                <a href="/emall/register">注册</a>
            `;
        }
    } catch (error) {
        // API 请求失败，默认显示未登录状态
        menuHTML += `
            <a href="/emall/login">登录</a>
            <a href="/emall/register">注册</a>
        `;
    }

    menuHTML += `</div></nav>`;
    menuContainer.innerHTML = menuHTML;
}

async function renderProductCard(itemIdx) {
    try {
        const itemInfo = await request(`/emall/require_safe?info=item&idx=${itemIdx}`);
        if (!itemInfo.result) return '';

        const card = document.createElement('div');
        card.className = 'product-card';
        card.innerHTML = `
            <img src="${API_BASE_URL}/emall/require_image?info=item&idx=${itemIdx}" alt="${itemInfo.name}">
            <div class="product-card-info">
                <h3><a href="/emall/item?idx=${itemIdx}" data-link>${itemInfo.name}</a></h3>
                <p>价格: ¥${parseFloat(itemInfo.price).toFixed(2)}</p>
                <p>店铺: <a href="/emall/shop?idx=${itemInfo.shop_idx}" data-link>${itemInfo.shop_name}</a></p>
                <p>卖家: <a href="/emall/user?idx=${itemInfo.seller_idx}" data-link>${itemInfo.seller_name}</a></p>
            </div>
        `;
        return card;
    } catch (error) {
        console.error(`Failed to render product card for idx ${itemIdx}:`, error);
        return null;
    }
}