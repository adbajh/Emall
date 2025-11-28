// FILE: /emall/js/item.js

async function renderItemPage() {
    const params = getQueryParams();
    const itemIdx = params.idx;
    if (!itemIdx) {
        document.getElementById('app').innerHTML = '<h1>无效的商品 ID</h1>';
        return;
    }

    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const itemInfo = await request(`/emall/require_safe?info=item&idx=${itemIdx}`);
        if (!itemInfo.result) throw new Error('商品信息获取失败');

        app.innerHTML = `
            <div class="container">
                <h2>${itemInfo.name}</h2>
                <img src="${API_BASE_URL}/emall/require_image?info=item&idx=${itemIdx}" alt="${itemInfo.name}" style="max-width: 400px; border-radius: 8px;">
                <p><strong>价格:</strong> ¥${parseFloat(itemInfo.price).toFixed(2)}</p>
                <p><strong>卖家:</strong> <a href="/emall/user?idx=${itemInfo.seller_idx}" data-link>${itemInfo.seller_name}</a></p>
                <p><strong>店铺:</strong> <a href="/emall/shop?idx=${itemInfo.shop_idx}" data-link>${itemInfo.shop_name}</a></p>
                <p><strong>发布于:</strong> ${itemInfo.publishDate} ${itemInfo.publishTime}</p>
                <p><strong>描述:</strong> ${itemInfo.description}</p>
                <p><strong>类别:</strong> ${itemInfo.types.join(', ')}</p>
                <div class="form-group">
                    <label for="quantity">数量:</label>
                    <input type="number" id="quantity" value="1" min="1" style="width: 100px;">
                </div>
                <div class="form-group">
                    <label for="address">收货地址:</label>
                    <input type="text" id="address" placeholder="请输入您的收货地址">
                </div>
                <button class="btn" id="buy-btn">立即购买</button>
                <button class="btn btn-secondary" id="add-to-cart-btn">加入购物车</button>
            </div>
        `;

        document.getElementById('buy-btn').addEventListener('click', () => createOrder('buy', itemIdx));
        document.getElementById('add-to-cart-btn').addEventListener('click', () => createOrder('add', itemIdx));

    } catch (error) {
        app.innerHTML = `<div class="container"><h2>商品加载失败</h2><p>${error.message}</p></div>`;
    }
}

async function createOrder(operation, itemIdx) {
    const quantity = document.getElementById('quantity').value;
    const address = document.getElementById('address').value;

    if (!quantity || !address) {
        alert('请输入数量和收货地址！');
        return;
    }

    try {
        const result = await request(`/emall/orders/create?item_idx=${itemIdx}`, 'POST', {
            quantity: parseInt(quantity),
            address: encodeURIComponent(address),
            operation
        });
        if (result.result) {
            alert(operation === 'buy' ? '购买成功！请前往订单页面支付。' : '成功加入购物车！');
            navigateTo('/emall/orders');
        } else {
            alert('操作失败：不能购买自己的商品。');
        }
    } catch (error) {
        alert('操作失败，请确认您已登录。');
    }
}