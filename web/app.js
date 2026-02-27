const uploadInput = document.getElementById('pdf-upload');
const uploadScreen = document.getElementById('upload-screen');
const editorScreen = document.getElementById('editor-screen');
const loadingIndicator = document.getElementById('loading-indicator');
const pagesContainer = document.getElementById('pages-container');
const saveBtn = document.getElementById('save-btn');
const closeBtn = document.getElementById('close-btn');

let currentFile = null;

uploadInput.addEventListener('change', async (e) => {
    if (e.target.files.length === 0) return;

    currentFile = e.target.files[0];
    uploadScreen.querySelector('label').classList.add('hidden');
    loadingIndicator.classList.remove('hidden');

    try {
        await loadAllPdfPages(currentFile);
    } catch (err) {
        console.error(err);
        alert('SYSTEM FAILURE: ' + err.message);
    } finally {
        loadingIndicator.classList.add('hidden');
        uploadScreen.querySelector('label').classList.remove('hidden');
    }
});

closeBtn.addEventListener('click', () => {
    uploadScreen.classList.remove('hidden');
    editorScreen.classList.add('hidden');
    pagesContainer.innerHTML = '';
    currentFile = null;
    uploadInput.value = '';
});

saveBtn.addEventListener('click', async () => {
    alert("COMPILE FUNCTION NOT YET IMPLEMENTED IN C++.");
});

async function loadAllPdfPages(file) {
    // 1. We first test total pages by asking for page 0
    const formData = new FormData();
    formData.append('file', file);
    formData.append('page', 0);

    const initialExtractRes = await fetch('/api/pdf/extract', {
        method: 'POST',
        body: formData
    });

    if (!initialExtractRes.ok) throw new Error('Failed to extract PDF initial metadata');
    const extractData = await initialExtractRes.json();
    const totalPages = extractData.total_pages;

    uploadScreen.classList.add('hidden');
    editorScreen.classList.remove('hidden');
    pagesContainer.innerHTML = ''; // clear

    // Loading all pages sequentially
    for (let i = 0; i < totalPages; i++) {
        await loadSinglePage(file, i);
    }
}

async function loadSinglePage(file, pageNum) {
    const formData = new FormData();
    formData.append('file', file);
    formData.append('page', pageNum);

    // Fetch Background Image
    const renderRes = await fetch('/api/pdf/render', {
        method: 'POST',
        body: formData
    });
    if (!renderRes.ok) throw new Error('Failed to render PDF page ' + pageNum);

    const blob = await renderRes.blob();
    const bgUrl = URL.createObjectURL(blob);

    // Fetch Text Blocks if we didn't already get them (we get page 0 blocks during intialization)
    const extractRes = await fetch('/api/pdf/extract', {
        method: 'POST',
        body: formData
    });
    if (!extractRes.ok) throw new Error('Failed to extract text for page ' + pageNum);

    const extractData = await extractRes.json();
    const textBlocks = extractData.blocks || [];

    await renderPageContainer(bgUrl, textBlocks, pageNum);
}

function renderPageContainer(bgUrl, textBlocks, pageNum) {
    return new Promise((resolve) => {
        const img = new Image();
        img.onload = () => {
            // Create wrapper
            const pageDiv = document.createElement('div');
            pageDiv.className = 'page-container';
            pageDiv.style.width = `${img.width}px`;
            pageDiv.style.height = `${img.height}px`;

            // Canvas
            const canvas = document.createElement('canvas');
            canvas.className = 'pdf-bg-canvas';
            canvas.width = img.width;
            canvas.height = img.height;
            const ctx = canvas.getContext('2d');
            ctx.drawImage(img, 0, 0);

            // Text layer
            const textLayer = document.createElement('div');
            textLayer.className = 'text-layer';
            textLayer.style.width = `${img.width}px`;
            textLayer.style.height = `${img.height}px`;

            const ZOOM = 2.0;

            textBlocks.forEach((block, idx) => {
                const input = document.createElement('div');
                input.contentEditable = true;
                input.className = 'text-overlay-input';
                input.spellcheck = false;

                const x = block.x * ZOOM;
                const y = block.y * ZOOM;
                const w = block.width * ZOOM;
                const h = block.height * ZOOM;

                input.style.left = `${x}px`;
                input.style.top = `${y}px`;
                input.style.width = `${w + 4}px`;
                input.style.height = `${h}px`;
                input.style.fontSize = `${h}px`;
                input.style.lineHeight = `${h}px`;
                input.innerText = block.text;

                // Store tracking info
                input.dataset.page = pageNum;
                input.dataset.idx = idx;

                input.addEventListener('focus', () => {
                    input.style.color = 'var(--crt-green)';
                    input.style.background = '#000';
                });

                input.addEventListener('blur', () => {
                    input.style.color = 'transparent';
                    input.style.background = 'transparent';
                });

                textLayer.appendChild(input);
            });

            pageDiv.appendChild(canvas);
            pageDiv.appendChild(textLayer);
            pagesContainer.appendChild(pageDiv);
            resolve();
        };
        img.src = bgUrl;
    });
}
