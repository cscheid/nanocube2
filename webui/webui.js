$(document).ready(function(){

  // create jsonwriter 
  var container = $('#jsoneditor')[0];
  var options = {
    mode: 'code',
  };
  var jsoneditor = new JSONEditor(container, options);

  // set default json
  var default_json = {
    '0': {
      'operation': 'find',
      'prefix': {'address':0, 'depth':0},
      'resolution': 2,
      'lowerBound': {'address':2, 'depth':3},
      'upperBound': {'address':6, 'depth':3}
    },
    '1': {
      'operation': 'all'
    }
  };
  jsoneditor.set(default_json);

  // query button
  $('#btn_query').on('click', function () {

    var query = jsoneditor.get();

    $.ajax({
      url: '/query',
      method: 'POST',
      dataType: 'json',
      data: JSON.stringify(query),
      success: function(json) {
        var str = JSON.stringify(json, null, 2);
        $('#result').text(str);
      }
    });

  });

  // rest button
  $('#btn_reset').on('click', function () {
    jsoneditor.set(default_json);
    $('#result').text("");
  });

});
