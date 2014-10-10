var accessData;

function init(data) {
  // things to do when our data has loaded
  accessData = data;
  renderChart();
}

function translate(x, y) {
  //positive x direction is to the right
  //positive y direction is down
  return "translate(" + x + ", " + y + ")";
}

// D3 configuration
var height = 500,
    width = 800,
    margins = [5,30,30,30]; //top, right, bottom, left

var rangePadding = 0.1,
    rangeOuterPadding = 0.3;

var xScale = d3.scale.linear().range([0,width - (margins[1] + margins[3])]);

var yScale = d3.scale.linear().range([0,height - (margins[0] + margins[2])]);

xScale.domain([1,507440]).nice();
xScale.domain([1,100000]).nice();
// yScale.domain([18446744073699066300,4194816]).nice();
yScale.domain([Math.pow(2,25), 3000000]).nice();

var fileColorScale = d3.scale.category10();

var siPrefixFormatter = d3.format("s");

var xAxis = d3.svg.axis()
    .scale(xScale)
    .orient("bottom")
    .tickFormat(siPrefixFormatter);

var yAxis = d3.svg.axis()
    .scale(yScale)
    .orient("left")
    .tickFormat(siPrefixFormatter);

var svg = d3.select("#accesses").append("svg")
    .attr("width", width)
    .attr("height", height)
    .classed("chart", true);

var marginContainer = svg.append("g")
    .attr("transform", translate(margins[3],margins[0]))
    .attr("width", width-margins[1]-margins[3])
    .attr("height", width-margins[0]-margins[2])
    .classed("margin-container", true);

function renderChart() {
  //we might be re-rendering, so remove the old
  d3.selectAll("main svg *:not(.margin-container)").remove();
  d3.selectAll("main svg .margin-container *").remove();

  marginContainer.append("g")
      .classed("axis", true)
      .classed("x", true)
      .attr("transform", translate(0, height - margins[0] - margins[2]))
      .call(xAxis);

  marginContainer.append("g")
      .classed("axis", true)
      .classed("y", true)
      // .attr("transform", translate(
      .call(yAxis);

  var dots = marginContainer.selectAll(".dot")
      .data(accessData)
    .enter().append("circle")
      .attr("class", "dot")
      .attr("r",1.5)
      .attr("cx", function(d) { return xScale(d.i); } )
      .attr("cy", function(d) { return yScale(d.addr); } )
      .attr("fill", function(d) { return fileColorScale(d.filename); } );

  d3.select("#loading").remove();
}

// Configure loading of coffeeData
d3.tsv('accesses.tsv')
  .row( function(d) {
    d.i = +d.i;
    d.addr = parseInt(d.addr);
    // if (d.filename != "perlbench.trace") { return null;}
    return d;
  })
  .get(function(error, data) {
    if (error) {
      console.log(error);
    } else {
      init(data);
    }
  });
