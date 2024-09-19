const width = 480;
const height = 270;

let script = {
    json: null,
    currentLine: 0,
    currentScene: 0,
}

let textbox = document.getElementById("text");
let textdiv = document.getElementById("text-div");
let frame = document.getElementById("frame");
let image = document.querySelector("img");
let video = document.querySelector("video");

getList().then(function(res) {
    response = JSON.parse(res);
    if(response.type === "list") {
	response.files.forEach((e) => {
	    textdiv.innerHTML += "<button onclick=\"init('"+e+"')\">"+e+"</button><br>";
	});
    } else {
	document.body.innerHTML += "error: "+response.msg;
    }
});

function add_img(name) {
    getBg(name).then(data => data.blob()).then(blob => {
	frame.innerHTML="<img src=\""+URL.createObjectURL(blob)+"\" style=\"max-height:"+height+"px;max-width:"+width+"px;\"/>";
	image = document.querySelector("img");
    });
}

function add_video(name) {
    getAnim(name).then(data => data.blob()).then(blob => {
	frame.innerHTML="<video src=\""+URL.createObjectURL(blob)+"\" type=\"video/mp4\" autoplay loop style=\"max-height:"+height+"px;max-width:"+width+"px;\"/>";
    	video = document.querySelector("video");
    });
}

function add_video_once(name) {
    getAnim(name).then(data => data.blob()).then(blob => {
	frame.innerHTML="<video src=\""+URL.createObjectURL(blob)+"\" type=\"video/mp4\" autoplay style=\"max-height:"+height+"px;max-width:"+width+"px;\"/>";
    	video = document.querySelector("video");
    });
}

function add_textbox() {
    textdiv.innerHTML="<button id=\"text\" onclick=\"parseLine()\"></button>";
    textbox = document.getElementById("text");
}

function init(name) {
    loadScript(name).then(function(res) {
	add_textbox();
	script.json = res;
	container.style = "max-width:"+width+"px;";
	frame.style = "max-height:"+height+"px;";
	parseLine();
    });
}

function parseLine() {
    let scene = script.json.scenes[script.currentScene];
    if(script.currentLine >= scene.script.length) {
	if(script.currentScene+1 === script.json.scenes.length) {
	    textdiv.innerHTML="END";
	    return;
	}
	script.currentScene++;
	script.currentLine = 0;
    }
    scene = script.json.scenes[script.currentScene];
    let line = scene.script[script.currentLine];
    if(line.includes("@bg")) {
	let variable = line.slice(line.indexOf(' ')+1);
	if(line.includes('loop')||line.includes('once'))
	    variable = variable.slice(0, variable.indexOf(' '));
	let file = '';
	if(variable.includes('.')) {
	    file = scene.resources_path+variable;
	}
       	else file = findResource(variable);
	if(file === '') {
	    document.body.innerHTML += "error: could not find variable "+variable;
	    return
	}
	if(file.includes('mp4')) {
	    if(line.includes('loop')) add_video(file);
	    else add_video_once(file);
	} else {
	    add_img(file);
	}
	script.currentLine++;
    }
    textbox.innerText = scene.script[script.currentLine++];
}

function loadScript(name) {
    return getScript(name).then(function(result) {
	response = JSON.parse(result);
	if(response.type === "script") {
	    return response;
	} else {
    	    document.body.innerHTML += "error: "+response.msg;
	}
    });
}

function findResource(name) {
    let global_resources = script.json.global_resources;
    let scene_resources = script.json.scenes[script.currentScene].resources;
    let resource = '';
    global_resources.some((e) => {
	for(key in e) if(key === name) resource = e[key];
	if(resource !== "") return;
    });
    if(resource !== "") return resource;
    scene_resources.some((e) => {
	for(key in e) if(key === name) resource = e[key];
	if(resource !== "") return;
    });
    return resource;
}

async function getList() {
    let response = await fetch('/list');
    let data = await response.text();
    return data;
}

async function getScript(name) {
    let response = await fetch('/script', {
	method: "POST",
	headers: {"Content-Type":"application/json"},
	body: JSON.stringify({"filename":name})
    });
    let data = await response.text();
    return data;
}

async function getAnim(name) {
    let response = await fetch('/anim', {
	method: "POST",
	headers: {"Content-Type":"application/json"},
	body: JSON.stringify({"filename":name})
    });
    let data = await response;
    return data;
}

async function getBg(name) {
    let response = await fetch('/bg', {
	method: "POST",
	headers: {"Content-Type":"application/json"},
	body: JSON.stringify({"filename":name})
    });
    let data = await response;
    return data;
}
