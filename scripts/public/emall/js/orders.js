// FILE: /emall/js/orders.js

const ORDER_STATUS_MAP = {
    0: '已下单未支付',
    1: '已支付未发货',
    2: '已发货未确认收货',
    3: '订单完成',
    4: '已退货未退款',
    5: '订单无效'
};

async function renderOrdersPage() {
    const app = document.getElementById('app');
    app.innerHTML = `
        <div class="container" style="display: flex;">
            <div id="order-sidebar" style="width: 200px; border-right: 1px solid #ccc; padding-right: 20px;">
                <h4>我创建的 (买家)</h4>
                <ul>
                    <li class="order-link" data-type="buyer" data-status="0">未支付</li>
                    <li class="order-link" data-type="buyer" data-status="1">待发货</li>
                    <li class="order-link" data-type="buyer" data-status="2">待收货</li>
                    <li class="order-link" data-type="buyer" data-status="3">已完成</li>
                    <li class="order-link" data-type="buyer" data-status="4">待退款</li>
                    <li class="order-link" data-type="buyer" data-status="5">无效订单</li>
                </ul>
                <h4>我收到的 (卖家)</h4>
                 <ul>
                    <li class="order-link" data-type="seller" data-status="1">待发货</li>
                    <li class="order-link" data-type="seller" data-status="2">待签收</li>
                    <li class="order-link" data-type="seller" data-status="3">已签收</li>
                    <li class="order-link" data-type="seller" data-status="4">待退款</li>
                    <li class="order-link" data-type="seller" data-status="5">无效订单</li>
                </ul>
            </div>
            <div id="order-content" style="flex-grow: 1; padding-left: 20px;">
                请选择一个分类查看订单。
            </div>
        </div>
        <style>
            .order-link { cursor: pointer; color: #007bff; margin-bottom: 5px; }
            .order-link:hover { text-decoration: underline; }
            .order-card { border: 1px solid #ddd; border-radius: 8px; padding: 15px; margin-bottom: 15px; }
        </style>
    `;
    
    document.querySelectorAll('.order-link').forEach(link => {
        link.addEventListener('click', (e) => {
            loadOrders(e.target.dataset.type, e.target.dataset.status);
        });
    });
}

async function loadOrders(type, status) {
    const content = document.getElementById('order-content');
    content.innerHTML = '加载中...';

    try {
        const me = await request('/emall/get_user_idx');
        const myIdx = me.user_idx;

        const searchBody = {
            // 根据 type ('buyer' 或 'seller') 设置 is_buyer
            is_buyer: (type === 'buyer'), 
            order_status: parseInt(status)
        };
        
        const ordersData = await request('/emall/orders/get', 'POST', searchBody);
        
        if (!ordersData.order_idx || ordersData.order_idx.length === 0) {
            content.innerHTML = '<h3>没有找到相关订单。</h3>';
            return;
        }

        let ordersHTML = '<h3>订单列表</h3>';
        for (let i = 0; i < ordersData.order_idx.length; i++) {
            const order = {
                idx: ordersData.order_idx[i],
                buyer_name: ordersData.buyer_name[i],
                seller_name: ordersData.seller_name[i],
                shop_name: ordersData.shop_name[i],
                item_name: ordersData.item_name[i],
                item_idx: ordersData.item_idx[i],
                quantity: ordersData.quantity[i],
                total_price: ordersData.total_price[i],
                address: ordersData.address[i],
                order_date: ordersData.order_date[i],
                order_time: ordersData.order_time[i],
                order_status: ordersData.order_status[i]
            };
            
            ordersHTML += `
                <div class="order-card">
                    <p><strong>商品:</strong> <a href="/emall/item?idx=${order.item_idx}" data-link>${order.item_name}</a> (x${order.quantity})</p>
                    <p><strong>总价:</strong> ¥${parseFloat(order.total_price).toFixed(2)}</p>
                    <p><strong>${type === 'buyer' ? '卖家' : '买家'}:</strong> ${type === 'buyer' ? order.seller_name : order.buyer_name}</p>
                    <p><strong>店铺:</strong> ${order.shop_name}</p>
                    <p><strong>收货地址:</strong> ${order.address}</p>
                    <p><strong>下单时间:</strong> ${order.order_date} ${order.order_time}</p>
                    <p><strong>订单状态:</strong> ${ORDER_STATUS_MAP[order.order_status]}</p>
                    <div>${generateOrderActions(order, type)}</div>
                </div>
            `;
        }
        content.innerHTML = ordersHTML;

        // 为新生成的按钮绑定事件
        document.querySelectorAll('.order-action-btn').forEach(button => {
            button.addEventListener('click', async e => {
                const { idx, operation } = e.target.dataset;
                if (confirm(`您确定要执行“${e.target.textContent}”操作吗？`)) {
                    try {
                        const result = await request(`/emall/orders/edit?idx=${idx}`, 'POST', { operation });
                        if (result.result) {
                            alert('操作成功！');
                            loadOrders(type, status); // 刷新列表
                        } else {
                            alert('操作失败。');
                        }
                    } catch (err) {
                        alert('操作时发生错误。');
                    }
                }
            });
        });

    } catch(error) {
        content.innerHTML = `加载订单失败: ${error.message}`;
    }
}

function generateOrderActions(order, userType) {
    const status = order.order_status;
    let actions = '';

    if (userType === 'buyer') {
        if (status === 0) {
            actions += `<button class="btn order-action-btn" data-idx="${order.idx}" data-operation="pay">支付</button> `;
            actions += `<button class="btn btn-secondary order-action-btn" data-idx="${order.idx}" data-operation="cancel">取消订单</button>`;
        } else if (status === 1 || status === 2) {
            actions += `<button class="btn btn-secondary order-action-btn" data-idx="${order.idx}" data-operation="return">申请退货</button>`;
        }
        if (status === 2) {
            actions += ` <button class="btn order-action-btn" data-idx="${order.idx}" data-operation="confirm">确认收货</button>`;
        }
    } else if (userType === 'seller') {
        if (status === 1) {
            actions += `<button class="btn order-action-btn" data-idx="${order.idx}" data-operation="send_out">发货</button>`;
        } else if (status === 4) {
            actions += `<button class="btn order-action-btn" data-idx="${order.idx}" data-operation="refund">确认退款</button>`;
        }
    }
    
    return actions || '<span>暂无可用操作</span>';
}