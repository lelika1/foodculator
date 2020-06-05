function getProductsRequest(success, fail = function(param) {}) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(JSON.parse(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('GET', '/get_ingredients', true);
    xhttp.send();
}

function addProductRequest(name, kcal, success, fail = function(param) {}) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(parseInt(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('POST', '/add_ingredient', true);
    xhttp.send(JSON.stringify({'product': name, 'kcal': kcal}));
}

function deleteProductRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the ingredient '${name}'?`)) {
        return;
    }

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(this.responseText);
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('DELETE', `/ingredient/${id}`, true);
    xhttp.send();
}

function getTablewareRequest(success, fail = function(param) {}) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(JSON.parse(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('GET', '/get_tableware', true);
    xhttp.send();
}

function addTablewareRequest(name, weight, success, fail = function(param) {}) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(parseInt(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('POST', '/add_tableware', true);
    xhttp.send(JSON.stringify({'name': name, 'weight': weight}));
}

function deleteTablewareRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the pot '${name}'?`)) {
        return;
    }

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(this.responseText);
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('DELETE', `/tableware/${id}`, true);
    xhttp.send();
}

function getRecipesRequest(success, fail = function(param) {}) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(JSON.parse(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('GET', '/get_recipes', true);
    xhttp.send();
}

function getRecipeRequest(id, success, fail = function(param) {}) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(JSON.parse(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('GET', `/recipe/${id}`, true);
    xhttp.send();
}

function addRecipeRequest(
    name, description, ingredients, success, fail = function(param) {}) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(parseInt(this.responseText));
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('POST', '/create_recipe', true);
    xhttp.send(JSON.stringify({
        'header': {'name': name},
        'description': description,
        'ingredients': ingredients,
    }));
}

function deleteRecipeRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the recipe '${name}'?`)) {
        return;
    }

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status == 200) {
                success(this.responseText);
            } else {
                fail(this.responseText);
            }
        }
    };

    xhttp.open('DELETE', `/recipe/${id}`, true);
    xhttp.send();
}
