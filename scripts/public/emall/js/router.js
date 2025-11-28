// FILE: /emall/js/router.js

const routes = {
    '/emall': 'renderMainPage',
    // '/emall/register': 'renderRegisterPage',
    // '/emall/login': 'renderLoginPage',
    '/emall/search': 'renderSearchPage',
    '/emall/item': 'renderItemPage',
    '/emall/shop': 'renderShopPage',
    '/emall/user': 'renderUserPage',
    '/emall/manage': 'renderManagePage',
    '/emall/manage/join': 'renderJoinShopPage',
    '/emall/manage/create': 'renderCreateShopPage',
    '/emall/manage/shop': 'renderShopManagementPage',
    '/emall/manage/shop/item': 'renderItemManagementPage', // For item editing
    '/emall/manage/shop/publish': 'renderPublishItemPage', // For item publishing
    '/emall/orders': 'renderOrdersPage',
    '/emall/messages': 'renderMessagesPage',
    '/emall/setting': 'renderSettingsPage',
    '/emall/user/temporary': 'renderTemporaryChatPage' // 临时聊天
};

function getPathSegments() {
    // 例如 /emall/manage/shop/123 -> ["emall", "manage", "shop", "123"]
    return window.location.pathname.split('/').filter(Boolean);
}

function getQueryParams() {
    const params = new URLSearchParams(window.location.search);
    const queryParams = {};
    for (const [key, value] of params.entries()) {
        queryParams[key] = value;
    }
    return queryParams;
}

// 辅助函数：解析路径参数
// 例如: /emall/manage/shop/123/item/456
// 返回: { shopIdx: 123, itemIdx: 456 }
function extractIdsFromPath(segments) {
    const ids = { shopIdx: null, itemIdx: null };
    
    // segments[0]='emall', [1]='manage', [2]='shop'
    if (segments.length >= 4) {
        ids.shopIdx = parseInt(segments[3]); // 获取 shop_idx
    }
    
    // 如果是 /item/456 结构
    if (segments.length >= 6 && segments[4] === 'item') {
        ids.itemIdx = parseInt(segments[5]); // 获取 item_idx
    }
    
    return ids;
}

function getPathSegments() {
    return window.location.pathname.split('/').filter(Boolean);
}


async function router() {
    const path = window.location.pathname;
    const appContainer = document.getElementById('app');
    appContainer.innerHTML = '<h1>加载中...</h1>';
    
    await renderMenuBar(); // 每次路由变化时都重新渲染菜单栏

    const segments = path.split('/').filter(Boolean); // ["emall", "manage", "shop", "1", "item", "2"]

    // --- 1. 匹配 /emall/manage/shop 相关路由 ---
    if (segments[0] === 'emall' && segments[1] === 'manage' && segments[2] === 'shop') {
        // 确保 shopIdx 存在
        if (!segments[3]) {
            appContainer.innerHTML = '<h1>404 - 缺少商店ID</h1>';
            return;
        }

        // 匹配: /emall/manage/shop/{shop_idx}/item/{item_idx} (商品管理)
        if (segments.length >= 6 && segments[4] === 'item') {
            await window.renderItemManagementPage();
            return;
        }

        // 匹配: /emall/manage/shop/{shop_idx}/publish (发布商品)
        if (segments.length >= 5 && segments[4] === 'publish') {
            await window.renderPublishItemPage();
            return;
        }

        // 匹配: /emall/manage/shop/{shop_idx} (商店管理详情)
        // 只要不是上面两种特殊情况，就是商店详情页
        await window.renderShopManagementPage();
        return;
    }

    // if (path === '/emall/temporary/user') {
    //     await window.renderTemporaryChatPage(); // 指向 messages.js 中的新函数
    //     return;
    // }

    // --- 2. 旧逻辑：处理其他标准页面 ---
    const key = path.split('?')[0].replace(/\/$/, '');
    let renderer;

    if (path.startsWith('/emall/item')) renderer = window.renderItemPage;
    else if (path.startsWith('/emall/shop')) renderer = window.renderShopPage;
    else if (path.startsWith('/emall/user')) renderer = window.renderUserPage;
    else renderer = window[routes[key]];

    if (renderer) {
        renderer();
    } else {
        appContainer.innerHTML = '<h1>404 - 页面未找到</h1>';
    }
}

function navigateTo(url) {
    history.pushState(null, null, url);
    router();
}

window.addEventListener('popstate', router);

document.addEventListener('DOMContentLoaded', () => {
    document.body.addEventListener('click', e => {
        if (e.target.matches('[data-link]')) {
            e.preventDefault();
            navigateTo(e.target.href);
        }
    });
    router();
});