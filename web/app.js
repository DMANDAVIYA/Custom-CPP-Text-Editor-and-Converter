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
    if (!currentFile) return;

    const modifications = [];
    const inputs = document.querySelectorAll('.text-overlay-input');

    inputs.forEach(input => {
        const text = input.innerText.trim();
        const orig = input.dataset.originalText;

        if (text !== orig) {
            modifications.push({
                page: parseInt(input.dataset.page),
                id: parseInt(input.dataset.idx), // Keep an ID for matching if needed
                x: parseFloat(input.dataset.x),
                y: parseFloat(input.dataset.y),
                width: parseFloat(input.dataset.width),
                height: parseFloat(input.dataset.height),
                text: text,
                orig: orig
            });
        }
    });

    if (modifications.length === 0) {
        alert("NO MODIFICATIONS DETECTED.");
        return;
    }

    try {
        saveBtn.innerText = ">> COMPILING... <<";

        const formData = new FormData();
        formData.append('file', currentFile);
        formData.append('modifications', JSON.stringify(modifications));

        const compileRes = await fetch('/api/pdf/compile', {
            method: 'POST',
            body: formData
        });

        if (!compileRes.ok) throw new Error(await compileRes.text());

        // Trigger download of the modified PDF
        const blob = await compileRes.blob();
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = "SYS_OVERRIDE_" + currentFile.name;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

    } catch (err) {
        console.error(err);
        alert("COMPILE ERROR: " + err.message);
    } finally {
        saveBtn.innerText = ">> COMPILE && SAVE_PDF <<";
    }
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
                input.dataset.originalText = block.text;
                input.dataset.x = block.x;
                input.dataset.y = block.y;
                input.dataset.width = block.width;
                input.dataset.height = block.height;

                input.addEventListener('focus', () => {
                    input.style.color = 'var(--crt-green)';
                    input.style.background = '#000';
                });

                input.addEventListener('blur', () => {
                    if (input.innerText.trim() !== input.dataset.originalText) {
                        // Text modified! Block out the original PDF image text behind it
                        input.style.color = '#000'; // Black text
                        input.style.background = '#fff'; // White background to cover original
                        input.style.border = '1px dashed var(--crt-green)'; // Keep it highlighted as edited
                    } else {
                        // Revert to invisible wrapper
                        input.style.color = 'transparent';
                        input.style.background = 'transparent';
                        input.style.border = '1px solid transparent';
                    }
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
