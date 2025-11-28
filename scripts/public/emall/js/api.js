// FILE: /emall/js/api.js

const API_BASE_URL = 'http://114.212.20.69:5555';

async function request(endpoint, method = 'GET', body = null, extraHeaders = {}) {
    const url = `${API_BASE_URL}${endpoint}`;
    
    const headers = {
        'UserAccount': sessionStorage.getItem('account') || '',
        'UserToken': sessionStorage.getItem('token') || '',
        'UserIp': '127.0.0.1', // 浏览器无法直接获取公网IP，此处为占位符
        ...extraHeaders
    };

    const options = {
        method,
        headers
    };

    if (body) {
        if (body instanceof FormData) {
            // FormData 会自动设置 Content-Type 为 multipart/form-data
            options.body = body;
        } else {
            headers['Content-Type'] = 'application/json';
            options.body = JSON.stringify(body);
        }
    }

    try {
        const response = await fetch(url, options);
        // 如果是图片请求，直接返回 response 对象
        if (endpoint.includes('/require_image')) {
            if (!response.ok) throw new Error('Image not found');
            return response.blob();
        }
        // 对于其他请求，解析 JSON
        const data = await response.json();
        if (!response.ok) {
            throw new Error(data.message || '请求失败');
        }
        return data;
    } catch (error) {
        console.error(`API Error on ${method} ${endpoint}:`, error);
        throw error;
    }
}