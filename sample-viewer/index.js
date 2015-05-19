var send = function() {
  var timer = null;
  var selectedLayer = null;
  var loading = document.getElementById('loading');
  loading.style.display = 'block';

  var showDetectproj = function(callback) {
    var postData =  document.getElementById('inputjson').value;
    getDetectProjJSON(postData, function(data) {
      if (data.status == 'Done') {
        document.getElementById('loading').style.display = 'none';
        if (timer) {
          clearInterval(timer);
          timer = null;
        }
        document.getElementById('input').style.display = 'none';
        document.getElementById('output').style.display = 'block';
        var map = createDetectprojMap('map', JSON.parse(postData).pyramid);

        var selectProj = document.getElementById('selectproj');
        selectProj.onchange = function(e) {
          var layer = e.target.options[e.target.selectedIndex].value;
          selectedLayer.setVisibility(false);
          selectedLayer = map.getLayersByName(layer)[0];
          selectedLayer.setVisibility(true);
        };
        for (var i = 0; i < data.projections.length; i++) {
          var option = document.createElement('option');
          var layerName = 'layer' + i;
          option.innerHTML = data.projections[i].proj4;
          option.value = layerName;
          selectProj.appendChild(option);
          var layer = createGEOJsonLayer(data.projections[i].geojson);
          layer.setName(layerName);
          layer.setVisibility(false);
          map.addLayer(layer);
        }
        selectedLayer = map.getLayersByName('layer0')[0];
        selectedLayer.setVisibility(true);
      } else if (data.status == 'Processed') {
        if (callback) {
          callback();
        }
      } else if (data.status == 'DetectprojError') {
        alert('Pro tuto mapu nie je možné spočítať projekciu.');
        console.log(data.message);
        document.getElementById('loading').style.display = 'none';
        if (timer) {
          clearInterval(timer);
          timer = null;
        }
      } else {
        alert('Někde se stala chyba. Kontaktujte nás, prosím, na mapy@mzk.cz.');
        console.error(data);
        document.getElementById('loading').style.display = 'none';
        if (timer) {
          clearInterval(timer);
          timer = null;
        }
      }
    });
  };
  showDetectproj(function() {
    if (timer == null) {
      timer = setInterval(showDetectproj, 15000);
    }
  });
}

function getDetectProjJSON(postData, callback) {
  var url = "http://detectproj.mzk.cz/v1";
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange = function() {
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
      var json = JSON.parse(xmlhttp.responseText);
      callback(json);
    }
  };
  xmlhttp.open("POST", url, true);
  xmlhttp.send(postData);
}

function createDetectprojMap(element, pyramid) {
  var zoomify = new OpenLayers.Layer.Zoomify("Zoomify" , pyramid.url,
        new OpenLayers.Size(pyramid.width, pyramid.height));
  if (pyramid.format == "Aware") {
    zoomify.getURL = getURLcustom;
  }

  var options = {
      controls: [],
      maxExtent: new OpenLayers.Bounds(0, 0, pyramid.width, pyramid.height),
      maxResolution: Math.pow(2, pyramid.n_tiers - 1),
      numZoomLevels: pyramid.n_tiers,
      units: 'pixels'
  };

  var map = new OpenLayers.Map(element, options);
  map.addControl(new OpenLayers.Control.Navigation);
  map.addControl(new OpenLayers.Control.ZoomPanel);
  map.addControl(new OpenLayers.Control.Attribution);

  var forwardTransform = function(point) {
          point.y = point.y - pyramid.height;
          return point;
  }
  var backwardTransform = function(point) {
          point.y = point.y + pyramid.height;
          return point;
  }
  OpenLayers.Projection.addTransform("RASTER", "EPSG:4326", forwardTransform);
  OpenLayers.Projection.addTransform("EPSG:4326", "RASTER", backwardTransform);

  map.addLayer(zoomify)
  map.setBaseLayer(zoomify);
  map.zoomToMaxExtent();
  return map;
}

function createGEOJsonLayer(geojson) {
  var geojsonFormat = new OpenLayers.Format.GeoJSON();
  var style = new OpenLayers.Style(
    {
      strokeColor: "#000000",
      strokeWidth: 1.5,
      label: '${getLabel}',
      fontColor: 'blue',
      fontSize: 20
    }, {
      context: {
        getLabel: function(a) {
          if (a.attributes.label) {
            return a.attributes.label;
          } else {
            return "";
          }
        }
      }
    });
  var layer = new OpenLayers.Layer.Vector("projection", {
    styleMap: new OpenLayers.StyleMap({default: style}),
    preFeatureInsert: function(feature) {
      return feature.geometry.transform(
        new OpenLayers.Projection("EPSG:4326"),
        new OpenLayers.Projection("RASTER")
      );
    }
  });
  layer.addFeatures(geojsonFormat.read(geojson));
  return layer;
}

function getURLcustom (bounds) {
  bounds = this.adjustBounds(bounds);
  var res = this.map.getResolution();
  var size = this.getImageSize(bounds);
  var x = bounds.left;
  var y = this.maxExtent.top - bounds.top;
  var z = this.numZoomLevels - this.map.getZoom();
  return this.url+"&res="+z+"&viewwidth="+size.w+"&viewheight="+size.h+"&x="+x+"&y="+y;
  var url = this.url;
  if (url instanceof Array) {
      url = this.selectUrl(path, url);
  }
  return url + path;
};
