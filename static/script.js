let script = [];
let path = '';
let imagePaths = [];
let animFolder = '';
let currentLine = 0;

let textbox = document.getElementById("text");
let textdiv = document.getElementById("text-div");
let frame = document.getElementById("frame");
let image = document.querySelector("img");
let video = document.querySelector("video");

getList().then(function(res) {
    list = JSON.parse(res);
    list.forEach((e) => {
	textdiv.innerHTML += "<button onclick=\"init('"+e+"')\">"+e+"</button><br>";
    });
});

function add_img(src) {
    frame.innerHTML="<img src=\""+src+"\"/>";
    image = document.querySelector("img");
}

function add_video(src) {
    frame.innerHTML="<video src=\""+src+"\" type=\"video/mp4\" autoplay loop/>";
    video = document.querySelector("video");
}

function add_video_once(src) {
    frame.innerHTML="<video src=\""+src+"\" type=\"video/mp4\" autoplay/>";
    video = document.querySelector("video");
}

function add_textbox() {
    textdiv.innerHTML="<button id=\"text\" onclick=\"parseLine()\"></button>";
    textbox = document.getElementById("text");
}

function init(name) {
    loadScript(name).then(function(res) {
	add_textbox();
	script = res;
	parseLine();
    });
}

function parseLine() {
    if(currentLine>=script.length) {
	textdiv.innerHTML="END";
	return;
    }
    if(currentLine==0) {
	let line = script[currentLine];
	const reg = /(\d+) (\d+) (\d+) (\S+)/;
	let toks = line.match(reg);
	let width = Number(toks[1]), height = Number(toks[2]), fps = Number(toks[3])
	let container = document.getElementById("container");
	container.style = "max-width:"+width+"px;";
	frame.style = "max-height:"+height+"px;";
	path = toks[4];
    	currentLine++;
    }
    let line = script[currentLine];
    if(line==="!scene") line=script[++currentLine];
    let read_images = false;
    if(line==="!bg") {
	read_images = true;
	line=script[++currentLine];
    }
    if(line==="!anim") {
	line=script[++currentLine];
	animFolder = line;
	add_video('/anim?'+path+animFolder);
	line=script[++currentLine];
    }
    if(line==="!anim_once" ||  line==="!anim_wait" || line==="!anim_text") {
	line=script[++currentLine];
	animFolder = line;
	add_video_once('/anim?'+path+animFolder);
	line=script[++currentLine];
    }
    if(read_images) {
	imagePaths = [];
	while(line!=="!script") {
	    imagePaths.push(line);
	    line=script[++currentLine];
	}
	add_img('/bg?'+path+imagePaths[0]);
    }
    if(line==="!script") line = script[++currentLine];
    line = script[currentLine];
    if(line.includes("@image")) {
	add_img('/bg?'+path+imagePaths[Number(line.slice(line.indexOf(' ')+1))]);
	currentLine++;
    }

    textbox.innerText = script[currentLine++];
}

function loadScript(name) {
    return getScript(name).then(function(result) {
	let split = result.split(/\r?\n/);
	for(let i=0; i<split.length; i++) {
	    split[i] = split[i].trim();
	}
	let out = split.filter(function(e) {return e!==""});
	return out;
    });
}

async function getList() {
    let response = await fetch('/list');
    let data = await response.text();
    return data;
}

async function getScript(name) {
    let response = await fetch('/script?'+name);
    let data = await response.text();
    return data;
}

async function getAnim(name) {
    let response = await fetch('/anim?'+name);
    let data = await response.text();
    return data;
}

async function getBg(name) {
    let response = await fetch('/bg?'+name);
    let data = await response.text();
    return data;
}
