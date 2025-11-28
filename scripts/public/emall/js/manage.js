// FILE: /emall/js/manage.js

// --- 我的商店主页 ---
async function renderManagePage() {
    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const myShops = await request('/emall/manage/get_shop_idx');
        let content = `
            <div class="container">
                <h2>我的商店</h2>
                <button class="btn" onclick="navigateTo('/emall/manage/create')">创建商店</button>
                <button class="btn btn-secondary" onclick="navigateTo('/emall/manage/join')">加入商店</button>
                <ul id="shop-list" style="list-style: none; padding: 0; margin-top: 20px;"></ul>
            </div>
        `;
        app.innerHTML = content;
        const shopList = document.getElementById('shop-list');
        if (myShops.shop_idx.length === 0) {
            shopList.innerHTML = '<li>您还没有创建或加入任何商店。</li>';
            return;
        }

        for (const shopIdx of myShops.shop_idx) {
            const shopInfo = await request(`/emall/require_safe?info=shop&idx=${shopIdx}`);
            if (shopInfo.result) {
                const li = document.createElement('li');
                li.style = "background-color: #f9f9f9; padding: 15px; border: 1px solid #ddd; margin-bottom: 10px; border-radius: 4px;";
                li.innerHTML = `
                    <strong><a href="/emall/manage/shop/${shopIdx}" data-link>${shopInfo.name}</a></strong>
                    ${shopInfo.manager_idx === myShops.user_idx ? '<span style="background-color: #28a745; color: white; padding: 2px 6px; border-radius: 4px; font-size: 12px; margin-left: 10px;">我创建的</span>' : ''}
                    <br>
                    <small>创建日期: ${shopInfo.date} | 经理: <a href="/emall/user?idx=${shopInfo.manager_idx}" data-link>${shopInfo.manager}</a></small>
                `;
                shopList.appendChild(li);
            }
        }
    } catch (error) {
        app.innerHTML = `<div class="container"><h2>页面加载失败</h2><p>${error.message}</p></div>`;
    }
}

// --- 加入商店 ---
function renderJoinShopPage() {
    const app = document.getElementById('app');
    app.innerHTML = `
        <div class="container">
            <a href="/emall/manage" data-link>&larr; 返回</a>
            <h2>加入商店</h2>
            <div class="form-group">
                <input type="text" id="invite-code" placeholder="输入邀请码">
                <button class="btn" id="search-invite-btn">搜索</button>
            </div>
            <div id="shop-info-result"></div>
        </div>
    `;

    document.getElementById('search-invite-btn').addEventListener('click', async () => {
        const inviteCode = document.getElementById('invite-code').value;
        if (!inviteCode) return;
        const resultContainer = document.getElementById('shop-info-result');
        try {
            const data = await request('/emall/manage/join', 'POST', { invite_code: inviteCode });
            if (data.shop_idx !== -1) {
                const shopInfo = await request(`/emall/require_safe?info=shop&idx=${data.shop_idx}`);
                resultContainer.innerHTML = `
                    <h4>${shopInfo.name}</h4>
                    <p>经理: ${shopInfo.manager}</p>
                    <button class="btn" id="confirm-join-btn">确认加入</button>
                `;
                document.getElementById('confirm-join-btn').addEventListener('click', async () => {
                    const confirmResult = await request('/emall/manage/confirm_join', 'POST', { invite_code: inviteCode });
                    if (confirmResult.result) {
                        alert('加入成功！');
                        navigateTo('/emall/manage');
                    } else {
                        alert('加入失败');
                    }
                });
            } else {
                resultContainer.innerHTML = '<p>邀请码不存在。</p>';
            }
        } catch (error) {
            resultContainer.innerHTML = '<p>搜索失败。</p>';
        }
    });
}

// --- 创建商店 ---
function renderCreateShopPage() {
    const app = document.getElementById('app');
    app.innerHTML = `
        <div class="container">
            <a href="/emall/manage" data-link>&larr; 返回</a>
            <h2>创建商店</h2>
            <form id="create-shop-form">
                <div class="form-group">
                    <label>商店名称</label>
                    <input type="text" id="shop-name" required>
                </div>
                <div class="form-group">
                    <label>邀请码</label>
                    <input type="text" id="invite-code" required>
                </div>
                <div class="form-group">
                    <label>店铺描述</label>
                    <textarea id="shop-description"></textarea>
                </div>
                <div class="form-group">
                    <label>店铺照片 (可选)</label>
                    <input type="file" id="shop-image" accept="image/*">
                </div>
                <button type="submit" class="btn">创建</button>
            </form>
        </div>
    `;

    document.getElementById('create-shop-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const headers = {
            name: encodeURIComponent(document.getElementById('shop-name').value),
            invite_code: document.getElementById('invite-code').value,
            description: encodeURIComponent(document.getElementById('shop-description').value)  
        };
        const imageFile = document.getElementById('shop-image').files[0];
        const formData = imageFile ? new FormData() : null;
        if (formData) {
            formData.append('photo', imageFile);
        }
        
        try {
            const result = await request('/emall/manage/create', 'POST', formData, headers);
            if (result.result) {
                alert('创建成功！');
                navigateTo('/emall/manage');
            } else {
                alert('创建失败：邀请码重复。');
            }
        } catch (error) {
            alert('创建商店时发生错误。');
        }
    });
}

// --- 商店管理详情页 ---
async function renderShopManagementPage() {
    // 使用 parseInt 截断 "123/publish" 这种可能存在的后缀，只取数字
    const segments = window.location.pathname.split('/');
    const shopIdx = parseInt(segments[4]); // /emall/manage/shop/{id} -> Index 4
    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const [me, shopInfo, myItems] = await Promise.all([
            request('/emall/get_user_idx'),
            request(`/emall/require_safe?info=shop&idx=${shopIdx}`),
            request(`/emall/manage/shop/${shopIdx}/get_item_idx`)
        ]);

        let managerActions = '';
        let memberActions = '';

        const inviteCodeHtml = `<p><strong>邀请码:</strong> ${shopInfo.invite_code || '获取失败'}</p>`;

        if (shopInfo.manager_idx === me.user_idx) {
            // 我是经理
            managerActions = `
                <button class="btn" id="edit-shop-btn">修改</button>
                <button class="btn btn-danger" id="dismiss-shop-btn">解散</button>
            `;
        } else {
            // 我是成员
            memberActions = `<button class="btn btn-danger" id="quit-shop-btn">退出</button>`;
        }

        app.innerHTML = `
            <div class="container">
                <a href="/emall/manage" data-link>&larr; 返回</a>
                <div id="shop-display">
                    <h2>${shopInfo.name}</h2>
                    <img src="${API_BASE_URL}/emall/require_image?info=shop&idx=${shopIdx}&t=${new Date().getTime()}" 
                        alt="${shopInfo.name}" 
                        style="max-width: 200px; border-radius: 8px;">
                    ${inviteCodeHtml}
                    <p><strong>描述:</strong> ${shopInfo.description}</p>
                </div>
                <div id="shop-edit-form" style="display:none;">
                    <!-- 编辑表单将在此处注入 -->
                </div>
                ${managerActions} ${memberActions}
            </div>
            <div class="container">
                <h3>我发布的商品</h3>
                <button class="btn" onclick="navigateTo('/emall/manage/shop/${shopIdx}/publish')">发布商品</button>
                <ul id="item-list" style="list-style:none; padding:0; margin-top:20px;"></ul>
            </div>
        `;

        const itemList = document.getElementById('item-list');
        if (myItems.item_idx.length === 0) {
            itemList.innerHTML = '<li>您还未在此商店发布任何商品。</li>';
        } else {
            for (const itemIdx of myItems.item_idx) {
                const itemInfo = await request(`/emall/require_safe?info=item&idx=${itemIdx}`);
                const li = document.createElement('li');
                li.style = "padding: 10px; border-bottom: 1px solid #eee;";
                // 跳转 URL 修改，注意格式: /shop?idx=.../item?idx=...
                li.innerHTML = `
                    <a href="/emall/manage/shop/${shopIdx}/item/${itemIdx}" data-link>${itemInfo.name}</a>
                    <span> - ¥${parseFloat(itemInfo.price).toFixed(2)}</span>
                    <small style="float:right;">${itemInfo.publishDate}</small>
                `;
                itemList.appendChild(li);
            }
        }

        // 绑定事件
        if (shopInfo.manager_idx === me.user_idx) {
            document.getElementById('edit-shop-btn').addEventListener('click', () => toggleShopEdit(true, shopInfo, shopIdx));
            document.getElementById('dismiss-shop-btn').addEventListener('click', async () => {
                if(confirm('确定要解散此商店吗？此操作不可逆！')) {
                    const result = await request(`/emall/manage/shop/${shopIdx}/dismiss`, 'DELETE');
                    if(result.result) { alert('解散成功'); navigateTo('/emall/manage'); }
                    else { alert('解散失败：商店内还有未完成的订单。'); }
                }
            });
        } else {
            document.getElementById('quit-shop-btn').addEventListener('click', async () => {
                if(confirm('确定要退出此商店吗？')) {
                    const result = await request(`/emall/manage/shop/${shopIdx}/quit`, 'DELETE');
                    if(result.result) { alert('退出成功'); navigateTo('/emall/manage'); }
                    else { alert('退出失败：您在该店还有未完成的订单。'); }
                }
            });
        }

    } catch (error) {
        app.innerHTML = `<div class="container"><h2>加载失败</h2><p>${error.message}</p></div>`;
    }
}

function toggleShopEdit(show, shopInfo, shopIdx) {
    const displayDiv = document.getElementById('shop-display');
    const formDiv = document.getElementById('shop-edit-form');
    // const shopIdx = shopInfo.shop_idx;

    if (show) {
        displayDiv.style.display = 'none';
        formDiv.innerHTML = `
            <div class="form-group">
                <label>商店名称</label>
                <input type="text" id="edit-shop-name" value="${shopInfo.name}">
            </div>
            <div class="form-group">
                <label>邀请码</label>
                <input type="text" id="edit-invite-code" value="${shopInfo.invite_code || ''}">
            </div>
            <div class="form-group">
                <label>店铺描述</label>
                <textarea id="edit-shop-description">${shopInfo.description}</textarea>
            </div>
            <div class="form-group">
                <label>更换照片</label>
                <input type="file" id="edit-shop-image">
            </div>
            <button class="btn" id="save-shop-btn">确认修改</button>
            <button type="button" class="btn btn-secondary" id="cancel-edit-btn">取消</button>
        `;
        formDiv.style.display = 'block';

        document.getElementById('save-shop-btn').addEventListener('click', async () => {
            const headers = {
                name: encodeURIComponent(document.getElementById('edit-shop-name').value),
                invite_code: document.getElementById('edit-invite-code').value,
                description: encodeURIComponent(document.getElementById('edit-shop-description').value)
            };
            const imageFile = document.getElementById('edit-shop-image').files[0];
            const formData = imageFile ? new FormData() : null;
            if (formData) formData.append('photo', imageFile);

            try {
                const result = await request(`/emall/manage/shop/${shopIdx}/edit`, 'POST', formData, headers);
                if(result.result) { alert('修改成功'); navigateTo(`/emall/manage/shop/${shopIdx}`); }
                else { alert('修改失败：邀请码重复。'); }
            } catch(e) { alert('修改时发生错误。'); }
        });
        document.getElementById('cancel-edit-btn').addEventListener('click', () => toggleShopEdit(false, shopInfo, shopIdx));
    } else {
        displayDiv.style.display = 'block';
        formDiv.style.display = 'none';
    }
}


// --- 商品发布页 ---
async function renderPublishItemPage() {
    const segments = window.location.pathname.split('/');
    const shopIdx = parseInt(segments[4]); // /emall/manage/shop/{id}/publish
    const app = document.getElementById('app');
    
    const tagStyle = `
    <style>
        .type-tag { display: inline-block; padding: 6px 15px; margin: 5px; border: 1px solid #ccc; border-radius: 20px; cursor: pointer; background-color: #f8f9fa; color: #333; user-select: none; }
        .type-tag.selected { background-color: #007bff; color: white; border-color: #007bff; }
    </style>`;

    // 修改 7: 返回链接
    app.innerHTML = tagStyle + `
        <div class="container">
            <a href="/emall/manage/shop/${shopIdx}" data-link>&larr; 返回</a>
            <h2>发布新商品</h2>
            <form id="publish-item-form">
                <div class="form-group"><label>商品名称</label><input type="text" id="item-name" required></div>
                <div class="form-group"><label>价格</label><input type="number" id="item-price" step="0.01" required></div>
                <div class="form-group"><label>商品描述</label><textarea id="item-description"></textarea></div>
                <div class="form-group">
                    <label>商品类别 (点击选择)</label>
                    <div id="item-types" style="border: 1px solid #eee; padding: 10px; border-radius: 4px;">加载中...</div>
                </div>
                <div class="form-group"><label>商品照片</label><input type="file" id="item-image" accept="image/*" required></div>
                <button type="submit" class="btn">发布</button>
            </form>
        </div>
    `;

    try {
        const typeData = await request('/emall/require_safe?info=type&idx=0');
        const typesContainer = document.getElementById('item-types');
        typesContainer.innerHTML = '';
        typeData.types.forEach(type => {
            const span = document.createElement('span');
            span.className = 'type-tag'; // 默认样式
            span.textContent = type;
            
            // 点击切换选中状态
            span.onclick = () => {
                span.classList.toggle('selected');
            };
            
            typesContainer.appendChild(span);
        });
        // typesContainer.innerHTML = typeData.types.map(type => `
        //     <label style="margin-right: 15px;"><input type="checkbox" name="item-type" value="${type}"> ${type}</label>
        // `).join('');
    } catch(e) { document.getElementById('item-types').innerHTML = '类别加载失败'; }
    
    document.getElementById('publish-item-form').addEventListener('submit', async (e) => {
        e.preventDefault();
        const imageFile = document.getElementById('item-image').files[0];
        
        const selectedTypes = Array.from(document.querySelectorAll('#item-types .type-tag.selected')).map(el => el.textContent);
        if(!imageFile) { alert('请上传商品照片'); return; }
        
        const formData = new FormData();
        formData.append('photo', imageFile);
        
        const headers = { 
            name: encodeURIComponent(document.getElementById('item-name').value),
            price: document.getElementById('item-price').value,
            description: encodeURIComponent(document.getElementById('item-description').value)
        };
        selectedTypes.forEach((type, i) => headers[`type${i}`] = encodeURIComponent(type));

        try {
            const result = await request(`/emall/manage/shop/${shopIdx}/publish`, 'POST', formData, headers);
            if(result.result) { alert('发布成功'); navigateTo(`/emall/manage/shop/${shopIdx}`); }
            else { alert('发布失败'); }
        } catch(err) { alert('发布时发生错误'); }
    });
}

// --- 商品管理页 ---
async function renderItemManagementPage() {
    const segments = window.location.pathname.split('/');
    const shopIdx = parseInt(segments[4]); 
    const itemIdx = parseInt(segments[6]); 
    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';
    
    try {
        const itemInfo = await request(`/emall/require_safe?info=item&idx=${itemIdx}`);
        const tagStyle = `
        <style>
            .type-tag { display: inline-block; padding: 6px 15px; margin: 5px; border: 1px solid #ccc; border-radius: 20px; cursor: pointer; background-color: #f8f9fa; color: #333; user-select: none; }
            .type-tag.selected { background-color: #007bff; color: white; border-color: #007bff; }
        </style>`;
        app.innerHTML = tagStyle + `
        <div class="container">
            <a href="/emall/manage/shop/${shopIdx}" data-link>&larr; 返回</a>
            <h2>管理商品：${itemInfo.name}</h2>
            <!-- 表单内容保持不变 -->
            <form id="edit-item-form">
                <div class="form-group"><label>商品名称</label><input type="text" id="item-name" value="${itemInfo.name}" required></div>
                <div class="form-group"><label>价格</label><input type="number" id="item-price" step="0.01" value="${parseFloat(itemInfo.price).toFixed(2)}" required></div>
                <div class="form-group"><label>商品描述</label><textarea id="item-description">${itemInfo.description}</textarea></div>
                <div class="form-group">
                    <label>商品类别 (点击选择)</label>
                    <div id="item-types" style="border: 1px solid #eee; padding: 10px; border-radius: 4px;">加载中...</div>
                </div>
                <div class="form-group"><label>更换照片</label><input type="file" id="item-image" accept="image/*"></div>
                <button type="submit" class="btn">确认修改</button>
                <button type="button" class="btn btn-danger" id="offshelf-btn">下架商品</button>
            </form>
        </div>
        `;
        
        const typeData = await request('/emall/require_safe?info=type&idx=0');
        const typesContainer = document.getElementById('item-types');
        typesContainer.innerHTML = '';
        typeData.types.forEach(type => {
            const span = document.createElement('span');
            
            // 检查当前商品是否包含该类别
            // 如果 itemInfo.types 数组包含当前 type，则加上 selected 类
            const isSelected = itemInfo.types.includes(type);
            
            span.className = `type-tag ${isSelected ? 'selected' : ''}`;
            span.textContent = type;
            
            // 点击交互
            span.onclick = () => span.classList.toggle('selected');
            
            typesContainer.appendChild(span);
        });
        // typesContainer.innerHTML = typeData.types.map(type => `
        //     <label style="margin-right: 15px;"><input type="checkbox" name="item-type" value="${type}" ${itemInfo.types.includes(type) ? 'checked' : ''}> ${type}</label>
        // `).join('');

        document.getElementById('edit-item-form').addEventListener('submit', async e => {
            e.preventDefault();
            const imageFile = document.getElementById('item-image').files[0];
            const selectedTypes = Array.from(document.querySelectorAll('#item-types .type-tag.selected')).map(el => el.textContent);

            const formData = imageFile ? new FormData() : null;
            if (formData) formData.append('photo', imageFile);

            const headers = {
                name: encodeURIComponent(document.getElementById('item-name').value),
                price: document.getElementById('item-price').value,
                description: encodeURIComponent(document.getElementById('item-description').value)
            };
            selectedTypes.forEach((type, i) => headers[`type${i}`] = encodeURIComponent(type));

            const result = await request(`/emall/manage/shop/${shopIdx}/item/${itemIdx}/edit`, 'POST', formData, headers);
            if(result.result) { alert('修改成功'); navigateTo(window.location.pathname + window.location.search); }
            else { alert('修改失败：该商品存在未完成的订单。'); }
        });

        document.getElementById('offshelf-btn').addEventListener('click', async () => {
            if(confirm('确定要下架此商品吗？')) {
                const result = await request(`/emall/manage/shop/${shopIdx}/item/${itemIdx}/offshelf`, 'DELETE');
                if(result.result) { alert('下架成功'); navigateTo(`/emall/manage/shop/${shopIdx}`); }
                else { alert('下架失败：该商品存在未完成的订单。'); }
            }
        });
    } catch(err) {
        app.innerHTML = `<div class="container"><h2>加载失败</h2></div>`;
        console.error(err);
    }
}