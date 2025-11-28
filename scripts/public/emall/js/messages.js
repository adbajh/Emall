// FILE: /emall/js/messages.js

// 全局状态管理
let chatState = {
    meIdx: -1,              // 当前登录用户ID
    currentChatUserIdx: -1, // 当前正在对话的目标ID
    
    // 时间戳管理
    // 用于向后轮询新消息 (初始化为1970，确保第一次能拉取到)
    lastTimestamp: { date: '1970-01-01', time: '00:00:00' }, 
    // 用于向前加载历史 (初始化为未来，确保第一次能拉取到历史)
    oldestTimestamp: { date: '9999-12-31', time: '23:59:59' }, 

    // 定时器
    msgPollingTimer: null,     // 消息轮询 (2s)
    contactPollingTimer: null, // 联系人列表轮询 (3s)
    
    // 标志位
    isLoadingHistory: false,
    hasMoreHistory: true,
    pollingTimer: null
};

// --- 入口函数 ---
async function renderMessagesPage() {
    resetPageLogic();
    console.log("开始加载...");
    console.log("最新时间：", chatState.lastTimestamp);
    console.log("历史时间：", chatState.oldestTimestamp);
    // 1. 获取当前用户ID
    try {
        const meData = await request('/emall/get_user_idx');
        chatState.meIdx = meData.user_idx;
    } catch (e) {
        console.error("未登录", e);
        return;
    }

    const app = document.getElementById('app');

    // 2. 注入样式 (CSS)
    const style = `
    <style>
        .chat-container { display: flex; height: calc(100vh - 120px); border: 1px solid #ddd; background: #fff; margin-top: 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.05); }
        
        /* 侧边栏样式 */
        .chat-sidebar { width: 260px; border-right: 1px solid #ddd; background: #f7f7f7; display: flex; flex-direction: column; }
        .sidebar-header { padding: 15px; background: #eee; font-weight: bold; border-bottom: 1px solid #ddd; height: 50px; box-sizing: border-box; display: flex; align-items: center; }
        .contact-list { flex: 1; overflow-y: auto; }
        .contact-item { display: flex; align-items: center; padding: 12px 15px; cursor: pointer; border-bottom: 1px solid #f0f0f0; transition: background 0.2s; }
        .contact-item:hover { background: #e9e9e9; }
        .contact-item.active { background: #cce5ff; }
        .contact-item img { width: 40px; height: 40px; border-radius: 50%; margin-right: 10px; object-fit: cover; }
        .contact-info { display: flex; flex-direction: column; flex: 1; overflow: hidden; }
        .contact-name { font-weight: bold; font-size: 14px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
        .contact-time { font-size: 12px; color: #999; margin-top: 2px; }
        .unread-badge { background-color: #ff3b30; color: white; border-radius: 10px; padding: 0 6px; font-size: 12px; height: 18px; line-height: 18px; margin-left: 5px; }

        /* 对话框样式 */
        .chat-main { flex: 1; display: flex; flex-direction: column; background: #f5f5f5; }
        .chat-header { padding: 0 15px; background: #fff; border-bottom: 1px solid #ddd; font-weight: bold; height: 50px; display: flex; align-items: center; }
        .message-box { flex: 1; padding: 20px; overflow-y: auto; display: flex; flex-direction: column; gap: 15px; }
        
        .message { max-width: 70%; padding: 10px 15px; border-radius: 8px; font-size: 14px; line-height: 1.5; word-wrap: break-word; position: relative; }
        .message.received { align-self: flex-start; background: #fff; border: 1px solid #e0e0e0; }
        .message.sent { align-self: flex-end; background: #95ec69; border: 1px solid #85d660; }
        
        .chat-input-area { padding: 15px; background: #fff; border-top: 1px solid #ddd; display: flex; gap: 10px; height: 80px; box-sizing: content-box; }
        .chat-input-area textarea { flex: 1; height: 100%; resize: none; padding: 10px; border: 1px solid #ddd; border-radius: 4px; outline: none; }
        .chat-input-area textarea:focus { border-color: #007bff; }
        
        /* Loading 提示 */
        .history-loading { text-align: center; font-size: 12px; color: #999; padding: 10px; }
    </style>
    `;

    // 3. 注入 HTML 结构
    app.innerHTML = style + `
        <div class="container chat-container">
            <!-- 侧边栏 -->
            <div class="chat-sidebar">
                <div class="sidebar-header">最近联系人</div>
                <div id="contact-list" class="contact-list">
                    <div style="padding:15px;color:#999;">加载中...</div>
                </div>
            </div>
            
            <!-- 主对话框 -->
            <div class="chat-main">
                <div id="chat-header" class="chat-header">请选择联系人</div>
                <div id="message-box" class="message-box"></div>
                <div class="chat-input-area">
                    <textarea id="msg-content" placeholder="输入消息... (Ctrl+Enter 发送)"></textarea>
                    <button class="btn" onclick="doSendMessage()">发送</button>
                </div>
            </div>
        </div>
    `;

    // 4. 绑定快捷键发送
    document.getElementById('msg-content').addEventListener('keydown', (e) => {
        if (e.ctrlKey && e.key === 'Enter') doSendMessage();
    });

    // 5. 解析 URL 参数
    const params = new URLSearchParams(window.location.search);
    const targetIdxStr = params.get('target_idx');
    let initTargetIdx = targetIdxStr ? parseInt(targetIdxStr) : null;

    console.log("对话人：", initTargetIdx);

    if (initTargetIdx) {
        chatState.currentChatUserIdx = initTargetIdx;
        // 同时更新缓存，保证下次点击菜单栏能回来
        sessionStorage.setItem('last_chat_target', initTargetIdx);
    }

    // 6. 初次加载联系人列表 (传入初始目标ID，处理临时会话注入)
    await loadContacts(initTargetIdx);

    // 7. 如果 URL 有目标，直接选中
    if (initTargetIdx && initTargetIdx !== chatState.meIdx) {
        // 尝试从列表获取名字，如果列表里没有(临时会话)，名字暂时显示 ID 或 Loading
        const item = document.querySelector(`.contact-item[data-uid="${initTargetIdx}"]`);
        const name = item ? item.querySelector('.contact-name').textContent : `User ${initTargetIdx}`;
        selectChat(initTargetIdx, name);
    }

    // 8. 开启侧边栏轮询 (1.5秒一次)
    if (chatState.contactPollingTimer) clearInterval(chatState.contactPollingTimer);
    chatState.contactPollingTimer = setInterval(() => loadContacts(chatState.currentChatUserIdx), 1500);
}

// --- 重置页面逻辑函数 ---
function resetPageLogic() {
    if (chatState.pollingTimer) clearInterval(chatState.pollingTimer); // 清理消息轮询
    if (chatState.contactPollingTimer) clearInterval(chatState.contactPollingTimer); // 清理联系人轮询
    
    // 重置核心状态
    chatState.currentChatUserIdx = -1;
    chatState.lastTimestamp = { date: '1970-01-01', time: '00:00:00' };
    chatState.oldestTimestamp = { date: '9999-12-31', time: '23:59:59' };
    chatState.isLoadingHistory = false;
    chatState.hasMoreHistory = true;
}

// --- 侧边栏逻辑 ---
async function loadContacts(activeTargetIdx) {
    try {
        // 获取列表 (后端应已按时间倒序排列)
        const res = await request('/emall/messages/get_history_liaison');
        let contacts = res.liaisons || [];
        const listDiv = document.getElementById('contact-list');
        
        // 【核心逻辑】检测 URL 中的 target_idx 是否在列表中
        // activeTargetIdx 通常来自 URL 或 当前选中的人
        if (activeTargetIdx && activeTargetIdx !== -1) {
            const exists = contacts.find(u => u.user_idx === activeTargetIdx);
            
            if (!exists) {
                // 如果不在列表中 (临时会话)，获取用户信息并手动注入到数组最前面
                try {
                    const userInfo = await request(`/emall/require_safe?info=user&idx=${activeTargetIdx}`);
                    if (userInfo.result) {
                        const tempContact = {
                            user_idx: activeTargetIdx,
                            name: userInfo.name,
                            unread: 0,
                            last_time: null // 标记为无历史时间
                        };
                        contacts.unshift(tempContact); // 插到最前面
                    }
                } catch (e) {
                    console.error("加载临时用户信息失败", e);
                }
            }
        }

        // 渲染列表
        listDiv.innerHTML = '';
        if (contacts.length === 0) {
            listDiv.innerHTML = '<div style="padding:15px; color:#999;">暂无联系人</div>';
            return;
        }

        contacts.forEach(u => {
            const div = document.createElement('div');
            // 高亮逻辑
            const isActive = (u.user_idx === chatState.currentChatUserIdx);
            div.className = `contact-item ${isActive ? 'active' : ''}`;
            div.dataset.uid = u.user_idx;
            
            // 点击事件：切换 URL 并切换聊天
            div.onclick = () => {
                // 修改 URL 但不刷新页面
                const newUrl = `/emall/messages?target_idx=${u.user_idx}`;
                window.history.pushState({path: newUrl}, '', newUrl);
                selectChat(u.user_idx, u.name);
            };

            const displayTime = u.last_time ? u.last_time.slice(0, 16) : '新对话';
            const badgeHtml = (u.unread > 0 && !isActive) 
                ? `<span class="unread-badge">${u.unread > 99 ? '99+' : u.unread}</span>` 
                : '';

            div.innerHTML = `
                <img src="${API_BASE_URL}/emall/require_image?info=user&idx=${u.user_idx}&t=${new Date().getTime()}">
                <div class="contact-info">
                    <span class="contact-name">${u.name}</span>
                    <span class="contact-time">${displayTime}</span>
                </div>
                ${badgeHtml}
            `;
            listDiv.appendChild(div);
        });

    } catch (e) {
        console.error("Load contacts failed", e);
    }
}

// --- 切换聊天对象逻辑 ---
async function selectChat(userIdx, userName) {
    // 避免重复点击
    // if (chatState.currentChatUserIdx === userIdx) return;

    // 1. 更新状态变量
    chatState.currentChatUserIdx = userIdx;
    
    // 2. 更新 UI 选中态
    document.querySelectorAll('.contact-item').forEach(el => el.classList.remove('active'));
    const activeItem = document.querySelector(`.contact-item[data-uid="${userIdx}"]`);
    if (activeItem) activeItem.classList.add('active');
    
    document.getElementById('chat-header').textContent = userName;
    const box = document.getElementById('message-box');
    box.innerHTML = ''; // 清空聊天框

    // 3. 重置时间戳和标志位
    // 3.1 停止旧的轮询
    if (chatState.msgPollingTimer) clearInterval(chatState.msgPollingTimer);
    
    // 3.2 准备加载历史
    chatState.hasMoreHistory = true;
    chatState.isLoadingHistory = false;
    
    // 3.3 重置时间戳
    // oldestTimestamp: 用于"滚轮向上"加载更早的消息
    chatState.oldestTimestamp = { date: '9999-12-31', time: '23:59:59' };
    // lastTimestamp: 用于"轮询"加载最新的消息 (初始设为0，加载历史后会更新)
    chatState.lastTimestamp = { date: '1970-01-01', time: '00:00:00' };

    // 4. 绑定滚动事件 (用于加载历史)
    box.onscroll = null;
    box.addEventListener('scroll', handleScroll);

    // 5. 调用后端：标记已读
    request('/emall/messages/mark_read', 'POST', { target_idx: userIdx }).catch(console.error);

    // 6. 【核心】获取初始历史消息
    // 参数 true 表示这是初始化加载
    await loadMoreHistory(true);

    // 7. 【核心】开启消息轮询 (每2秒获取一次新消息)
    chatState.msgPollingTimer = setInterval(() => pollMessages(userIdx), 500);
}

// --- 滚轮检测与历史加载逻辑 ---
function handleScroll(e) {
    const box = e.target;
    // 滚轮到最上方 (scrollTop === 0) 且 没在加载 且 还有更多
    if (box.scrollTop === 0 && !chatState.isLoadingHistory && chatState.hasMoreHistory) {
        loadMoreHistory(false); // false 表示这是向上加载，不是初始化
    }
}

async function loadMoreHistory(isInitialLoad) {
    chatState.isLoadingHistory = true;
    const box = document.getElementById('message-box');

    // 记录加载前的高度，用于修正滚动位置
    const oldScrollHeight = box.scrollHeight;

    // 显示 Loading
    let loadingEl = null;
    if (!isInitialLoad) {
        loadingEl = document.createElement('div');
        loadingEl.className = 'history-loading';
        loadingEl.innerText = '加载历史消息...';
        box.prepend(loadingEl);
    }

    try {
        const payload = { target_idx: chatState.currentChatUserIdx };
        
        // 如果不是初始化，带上 oldestTimestamp，获取比这个时间更早的消息
        if (!isInitialLoad) {
            payload.before_date = chatState.oldestTimestamp.date;
            payload.before_time = chatState.oldestTimestamp.time;
        }

        // 发送请求
        const res = await request('/emall/messages/receive_history', 'POST', payload);

        // 移除 Loading
        if (loadingEl) loadingEl.remove();

        if (res.result && res.messages && res.messages.length > 0) {
            // 1. 更新最远一次获取时间 (oldestTimestamp) 为返回的第一条(最老)消息的时间
            const oldestMsg = res.messages[0];
            chatState.oldestTimestamp = { date: oldestMsg.date, time: oldestMsg.time };

            // 2. 如果是初始化加载，同时更新最近一次获取时间 (lastTimestamp) 为最后一条(最新)消息的时间
            // 这样轮询器就知道从哪里开始查新消息
            if (isInitialLoad) {
                const newestMsg = res.messages[res.messages.length - 1];
                chatState.lastTimestamp = { date: newestMsg.date, time: newestMsg.time };
            }

            // 3. 渲染并插入顶部
            // res.messages 已经是正序 (Old -> New)
            const fragment = document.createDocumentFragment();
            res.messages.forEach(msg => {
                const div = createMessageDiv(msg);
                fragment.appendChild(div);
            });
            box.prepend(fragment);

            // 4. 修正滚动条位置
            if (isInitialLoad) {
                // 初始化：直接滚到底部看最新消息
                scrollToBottom();
            } else {
                // 向上加载：保持视口位置不变
                // 新位置 = (新总高度 - 旧总高度)
                const newScrollHeight = box.scrollHeight;
                box.scrollTop = newScrollHeight - oldScrollHeight;
            }
        } else {
            // 没数据了
            if (!isInitialLoad) {
                chatState.hasMoreHistory = false;
            } else {
                // 这是一个全新的聊天，没有任何历史
                // 必须把 lastTimestamp 更新为当前时间，防止轮询出 bug
                const now = new Date();
                const d = now.toISOString().split('T')[0];
                const t = now.toTimeString().split(' ')[0];
                chatState.lastTimestamp = { date: d, time: t };
            }
        }
    } catch (e) {
        console.error("Load history failed", e);
    } finally {
        chatState.isLoadingHistory = false;
    }
}

// --- 轮询新消息逻辑 ---
async function pollMessages(targetIdx) {
    try {
        // 请求参数：获取比 lastTimestamp 更新的消息
        const res = await request('/emall/messages/receive_latest', 'POST', {
            target_idx: targetIdx,
            last_date: chatState.lastTimestamp.date,
            last_time: chatState.lastTimestamp.time
        });

        if (res.result && res.messages && res.messages.length > 0) {
            // 渲染并插入底部
            res.messages.forEach(msg => appendMessage(msg));
            
            // 更新最近一次获取时间 (lastTimestamp) 为最后一条新消息的时间
            const lastMsg = res.messages[res.messages.length - 1];
            chatState.lastTimestamp = { date: lastMsg.date, time: lastMsg.time };
            
            // 自动滚到底部
            scrollToBottom();
        }
    } catch (e) { console.error("Poll error", e); }
}

// --- 发送消息逻辑 ---
async function doSendMessage() {
    if (chatState.currentChatUserIdx === -1) {
        alert("请选择联系人");
        return;
    }
    
    const input = document.getElementById('msg-content');
    const content = input.value.trim();
    if (!content) return;

    // 1. 乐观更新 UI (直接显示在底部)
    const tempMsg = {
        sender_idx: chatState.meIdx,
        content: content,
        date: '', time: '' // 暂时为空，等轮询或下次加载
    };
    appendMessage(tempMsg);
    scrollToBottom();
    input.value = '';

    // 2. 发送请求
    try {
        await request('/emall/messages/send', 'POST', {
            target_idx: chatState.currentChatUserIdx,
            content: content
        });
        // 发送成功后不需要手动更新时间戳，
        // 如果后端 receive_latest 包含自己发的消息，轮询会自动拉取并更新时间戳
        // 如果后端不包含，这里已经显示了。
    } catch (e) {
        alert("发送失败");
    }
}

// 辅助函数：创建消息DOM
function createMessageDiv(msg) {
    const div = document.createElement('div');
    const isMe = (msg.sender_idx === chatState.meIdx);
    div.className = `message ${isMe ? 'sent' : 'received'}`;
    div.textContent = msg.content;
    div.title = `${msg.date} ${msg.time}`;
    return div;
}

// 辅助函数：追加消息到底部
function appendMessage(msg) {
    const box = document.getElementById('message-box');
    const div = createMessageDiv(msg);
    box.appendChild(div);
}

// 辅助函数：滚到底部
function scrollToBottom() {
    const box = document.getElementById('message-box');
    box.scrollTop = box.scrollHeight;
}