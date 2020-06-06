function focusOn(id) {
    $('#' + id[0]).focus();
}

function callOnEnter(fn, ...params) {
    if (event.key === 'Enter') {
        fn(params);
    }
}
