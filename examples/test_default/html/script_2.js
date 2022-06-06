function draw2()
{
	let canvas = document.getElementById("canvas2");
	let ctx = canvas.getContext("2d");
	ctx.font = "34px Arial Bold";

	function ft(s, x, y) {ctx.fillText(s, x, y);}
	function fr(x, y, w, h) {ctx.fillRect(x, y, w, h);}

	function fs(c) {ctx.fillStyle = c;}

	fs("#EAECEE");
	fr(0, 0, 200, 50);
	fs("#D5D8DC");
	fr(200, 0, 200, 50);
	fs("#ABEBC6");
	fr(450, 0, 200, 50);
	fs("#A3E4D7");
	fr(650, 00, 200, 50);
	fs("#93D8EF");
	fr(0, 50, 200, 50);
	fs("#93D8EF");
	fr(200, 50, 200, 50);
	fs("#B1C2F8");
	fr(450, 50, 200, 50);
	fs("#B1C2F8");
	fr(650, 50, 200, 50);
	fs("#F5B7B1");
	fr(0, 100, 200, 50);
	fs("#F5B7B1");
	fr(200, 100, 200, 50);
	fs("#D98880");
	fr(450, 100, 200, 50);
	fs("#D98880");
	fr(650, 100, 200, 50);
	fs("#CB4335");
	fr(900, 100, 200, 50);
	fs("#CB4335");
	fr(1100, 100, 200, 50);
	fs("black");
	ft("Literal",10,35)
	ft("Literal",210,35)
	ft("Match",460,35)
	ft("Match",660,35)
	ft("ShortRepeat",10,85)
	ft("ShortRepeat",210,85)
	ft("LongRepeat0",460,85)
	ft("LongRepeat0",660,85)
	ft("LongRepeat1",10,135)
	ft("LongRepeat1",210,135)
	ft("LongRepeat2",460,135)
	ft("LongRepeat2",660,135)
	ft("LongRepeat3",910,135)
	ft("LongRepeat3",1110,135)
}