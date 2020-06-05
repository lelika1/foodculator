function getProductsRequest(success, fail = function(param) {}) {
    $.get('/get_ingredients')
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function addProductRequest(name, kcal, success, fail = function(param) {}) {
    $.post('/add_ingredient', JSON.stringify({'product': name, 'kcal': kcal}))
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function deleteProductRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the ingredient '${name}'?`)) {
        return;
    }

    $.ajax({type: 'DELETE', url: `/ingredient/${id}`})
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function getTablewareRequest(success, fail = function(param) {}) {
    $.get('/get_tableware')
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function addTablewareRequest(name, weight, success, fail = function(param) {}) {
    $.post('/add_tableware', JSON.stringify({'name': name, 'weight': weight}))
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function deleteTablewareRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the pot '${name}'?`)) {
        return;
    }

    $.ajax({type: 'DELETE', url: `/tableware/${id}`})
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function getRecipesRequest(success, fail = function(param) {}) {
    $.get('/get_recipes')
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function getRecipeRequest(id, success, fail = function(param) {}) {
    $.get(`/recipe/${id}`)
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function addRecipeRequest(
    name, description, ingredients, success, fail = function(param) {}) {
    $.post('/create_recipe', JSON.stringify({
         'header': {'name': name},
         'description': description,
         'ingredients': ingredients,
     }))
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}

function deleteRecipeRequest(name, id, success, fail = function(param) {}) {
    if (!confirm(`Do you want to delete the recipe '${name}'?`)) {
        return;
    }

    $.ajax({type: 'DELETE', url: `/recipe/${id}`})
        .done(data => success(data))
        .fail(data => fail(data.responseText));
}
