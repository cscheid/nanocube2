jQuery(function() {

  $('#btn_query').on('click', function() {
    // clear result first
    $('#result').html("");

    // hard code a query for now
    var query = {
      '0': {
        'operation': 'find',
        'prefix': {'address':0, 'depth':0},
        'resolution': 2,
        'lowerBound': {},
        'upperBound': {}
      },
      '1': {
        'operation': 'all'
      }
    };

    $.ajax({
      url: '/query',
      method: 'POST',
      dataType: 'json',
      data: JSON.stringify(query),
      success: function(json) {
        console.log(json.result);
        $('#result').html(JSON.stringify(json.result));
      }
    });
  });

});
