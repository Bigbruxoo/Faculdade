from flask import Flask, request
import subprocess

app = Flask(__name__)

HTML_TEMPLATE = '''
<!DOCTYPE html>
<html lang="pt-br">
<head>
    <meta charset="UTF-8">
    <title>Sistema de Busca de Acervo</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css">
</head>
<body class="bg-light">
<div class="container mt-5">
    <div class="card shadow">
        <div class="card-header bg-primary text-white">
            <h2>Sistema de Busca de Acervo</h2>
        </div>
        <div class="card-body">
            <form action="/buscar" method="post" class="row g-3">
                <div class="col-md-4">
                    <label for="tipo" class="form-label">Tipo de busca:</label>
                    <select name="tipo" id="tipo" class="form-select">
                        <option value="titulo">Título</option>
                        <option value="autor">Autor</option>
                        <option value="codigo">Código</option>
                    </select>
                </div>
                <div class="col-md-6">
                    <label for="query" class="form-label">Digite sua busca:</label>
                    <input type="text" id="query" name="query" class="form-control" required>
                </div>
                <div class="col-md-2 d-flex align-items-end">
                    <button type="submit" class="btn btn-success w-100">Buscar</button>
                </div>
            </form>
            {resultados}
        </div>
    </div>
</div>
</body>
</html>
'''

@app.route('/', methods=['GET'])
def home():
    return HTML_TEMPLATE.format(resultados="")

@app.route('/buscar', methods=['POST'])
def buscar():
    tipo = request.form['tipo']
    query = request.form['query']

    try:
        result = subprocess.run(
            ['.\\main.exe', 'ACERVO-DAS-BIBLIOTECAS-IFES-2025.csv', tipo, query],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        if result.returncode != 0:
            resultados = f'<div class="alert alert-danger mt-4"><pre>{result.stderr}</pre></div>'
        else:
            # Formata a saída para HTML
            saida = result.stdout.replace('\n', '<br>')
            resultados = f'<div class="alert alert-info mt-4"><pre>{saida}</pre></div>'

        return HTML_TEMPLATE.format(resultados=resultados)
    except Exception as e:
        return HTML_TEMPLATE.format(resultados=f'<div class="alert alert-danger mt-4"><pre>{str(e)}</pre></div>')

if __name__ == '__main__':
    app.run(debug=True)
