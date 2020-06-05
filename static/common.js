function setFocusOnNextInput(current) {
    if (current.length != 1) {
        return;
    }
    $(':input:eq(' + ($(':input').index(current[0]) + 1) + ')').focus();
}

function callOnEnter(fn, ...params) {
    if (event.key === 'Enter') {
        fn(params);
    }
}
