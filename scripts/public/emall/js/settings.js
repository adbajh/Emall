// FILE: /emall/js/settings.js

async function renderSettingsPage() {
    const app = document.getElementById('app');
    app.innerHTML = '<div class="container">加载中...</div>';

    try {
        const me = await request('/emall/get_user_idx');
        const userInfo = await request(`/emall/require_safe?info=user&idx=${me.user_idx}`);

        // 渲染页面结构：包含一个展示区和三个隐藏的表单区
        app.innerHTML = `
            <!-- 1. 主展示区 -->
            <div id="settings-main" class="container">
                <h2>账户设置</h2>
                <div style="display: flex; align-items: center; margin-bottom: 20px;">
                    <img src="${API_BASE_URL}/emall/require_image?info=user&idx=${me.user_idx}&t=${new Date().getTime()}" 
                         style="width: 100px; height: 100px; border-radius: 50%; object-fit: cover; margin-right: 20px;">
                    <div>
                        <p><strong>名称:</strong> ${userInfo.name}</p>
                        <p><strong>账号:</strong> ${userInfo.account}</p>
                        <p><strong>电话:</strong> ${userInfo.phone || '未设置'}</p>
                        <p><strong>邮箱:</strong> ${userInfo.email || '未设置'}</p>
                        <p><strong>描述:</strong> ${userInfo.description || '暂无'}</p>
                    </div>
                </div>
                
                <hr>
                <div style="display: flex; gap: 10px; flex-wrap: wrap;">
                    <button class="btn" id="btn-to-info">修改个人信息</button>
                    <button class="btn" id="btn-to-account">修改账号</button>
                    <button class="btn" id="btn-to-password">修改密码</button>
                    <button class="btn btn-secondary" id="btn-logout">登出</button>
                    <button class="btn btn-danger" id="btn-delete">删除账号</button>
                </div>
            </div>

            <!-- 2. 修改个人信息表单 (隐藏) -->
            <div id="form-info-section" class="container" style="display:none;">
                <h3>修改个人信息</h3>
                <form id="form-info">
                    <div class="form-group"><label>名称</label><input type="text" id="edit-name" value="${userInfo.name}"></div>
                    <div class="form-group"><label>电话</label><input type="text" id="edit-phone" value="${userInfo.phone || ''}"></div>
                    <div class="form-group"><label>邮箱</label><input type="email" id="edit-email" value="${userInfo.email || ''}"></div>
                    <div class="form-group"><label>描述</label><textarea id="edit-description">${userInfo.description || ''}</textarea></div>
                    <div class="form-group"><label>更换头像</label><input type="file" id="edit-photo" accept="image/*"></div>
                    <button type="submit" class="btn">保存</button>
                    <button type="button" class="btn btn-secondary btn-cancel">取消</button>
                </form>
            </div>

            <!-- 3. 修改账号表单 (隐藏) -->
            <div id="form-account-section" class="container" style="display:none;">
                <h3>修改账号</h3>
                <p class="text-muted">修改账号需要验证您的密码。</p>
                <form id="form-account">
                    <div class="form-group"><label>当前密码</label><input type="password" id="acc-verify-pass" required></div>
                    <div class="form-group"><label>新账号</label><input type="text" id="acc-new-account" required></div>
                    <button type="submit" class="btn">确认修改</button>
                    <button type="button" class="btn btn-secondary btn-cancel">取消</button>
                </form>
            </div>

            <!-- 4. 修改密码表单 (隐藏) -->
            <div id="form-password-section" class="container" style="display:none;">
                <h3>修改密码</h3>
                <form id="form-password">
                    <div class="form-group"><label>旧密码</label><input type="password" id="pwd-old" required></div>
                    <div class="form-group"><label>新密码</label><input type="password" id="pwd-new" required></div>
                    <div class="form-group"><label>确认新密码</label><input type="password" id="pwd-confirm" required></div>
                    <button type="submit" class="btn">确认修改</button>
                    <button type="button" class="btn btn-secondary btn-cancel">取消</button>
                </form>
            </div>
        `;

        // --- 事件绑定 ---

        // 切换显示的辅助函数
        const showSection = (sectionId) => {
            ['settings-main', 'form-info-section', 'form-account-section', 'form-password-section'].forEach(id => {
                document.getElementById(id).style.display = (id === sectionId) ? 'block' : 'none';
            });
        };

        // 按钮跳转
        document.getElementById('btn-to-info').onclick = () => showSection('form-info-section');
        document.getElementById('btn-to-account').onclick = () => showSection('form-account-section');
        document.getElementById('btn-to-password').onclick = () => showSection('form-password-section');
        
        // 所有取消按钮
        document.querySelectorAll('.btn-cancel').forEach(btn => {
            btn.onclick = () => {
                // 重置表单并返回
                document.getElementById('form-info').reset();
                document.getElementById('form-account').reset();
                document.getElementById('form-password').reset();
                showSection('settings-main');
            };
        });

        // 1. 处理修改个人信息
        document.getElementById('form-info').addEventListener('submit', async (e) => {
            e.preventDefault();
            const headers = {
                name: encodeURIComponent(document.getElementById('edit-name').value),
                phone_number: document.getElementById('edit-phone').value,
                email: document.getElementById('edit-email').value,
                description: encodeURIComponent(document.getElementById('edit-description').value)
            };
            
            const photoFile = document.getElementById('edit-photo').files[0];
            const formData = photoFile ? new FormData() : null;
            if (formData) formData.append('photo', photoFile);

            try {
                // API 2.21 修改：只传 name, phone, email, description 到 header，不传 account/password
                const result = await request('/emall/setting/edit', 'POST', formData, headers);
                if(result.result) {
                    alert('个人信息修改成功！');
                    navigateTo('/emall/setting');
                } else {
                    alert('修改失败。');
                }
            } catch(err) { alert('请求出错：' + err.message); }
        });

        // 2. 处理修改账号
        document.getElementById('form-account').addEventListener('submit', async (e) => {
            e.preventDefault();
            const password = document.getElementById('acc-verify-pass').value;
            const new_account = document.getElementById('acc-new-account').value;

            try {
                // 新增 API (1)
                const result = await request('/emall/setting/edit_account', 'POST', { password, new_account });
                if(result.result) {
                    alert('账号修改成功！');
                    // 关键：修改成功后，必须更新本地存储的 account，否则后续请求会失败
                    sessionStorage.setItem('account', new_account);
                    navigateTo('/emall/setting');
                } else {
                    alert('修改失败：可能是密码错误或新账号已被占用。');
                }
            } catch(err) { alert('请求出错：' + err.message); }
        });

        // 3. 处理修改密码
        document.getElementById('form-password').addEventListener('submit', async (e) => {
            e.preventDefault();
            const old_password = document.getElementById('pwd-old').value;
            const new_password = document.getElementById('pwd-new').value;
            const confirm_password = document.getElementById('pwd-confirm').value;

            if (new_password !== confirm_password) {
                alert('两次输入的新密码不一致！');
                return;
            }

            try {
                // 新增 API (2)
                const result = await request('/emall/setting/edit_password', 'POST', { old_password, new_password });
                if(result.result) {
                    alert('密码修改成功！请重新登录。');
                    // 密码修改通常要求重新登录，这里清除 token
                    sessionStorage.removeItem('account');
                    sessionStorage.removeItem('token');
                    navigateTo('/emall/login'); // 假设有登录页路由
                } else {
                    alert('修改失败：旧密码错误。');
                }
            } catch(err) { alert('请求出错：' + err.message); }
        });

        // 4. 登出
        document.getElementById('btn-logout').addEventListener('click', async () => {
            if (confirm('您确定要登出吗？')) {
                const result = await request('/emall/setting/logout', 'Post');
                if (result.result) {
                    alert('登出成功！');
                    sessionStorage.removeItem('account');
                    sessionStorage.removeItem('token');
                    navigateTo('/emall');
                } else { alert('登出失败。'); }
            }
        });

        // 5. 删除账号
        document.getElementById('btn-delete').addEventListener('click', async () => {
            if (confirm('警告：此操作不可逆！您确定要删除您的账号吗？')) {
                const result = await request('/emall/setting/delete', 'Delete');
                if (result.result) {
                    alert('账号删除成功！');
                    sessionStorage.removeItem('account');
                    sessionStorage.removeItem('token');
                    navigateTo('/emall');
                } else { alert('删除失败：您还有未完成的订单。'); }
            }
        });

    } catch (error) {
        app.innerHTML = '<div class="container">无法加载设置页面，请稍后重试。</div>';
        console.error(error);
    }
}