#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from flask import Flask, render_template, request, jsonify, send_file
from flask_cors import CORS
import subprocess
import os
import json
import time
from datetime import datetime

app = Flask(__name__)
CORS(app)

# 配置
UPLOAD_FOLDER = 'uploads'
RESULT_FOLDER = 'test_results'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(RESULT_FOLDER, exist_ok=True)

tasks = {}

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/analyze', methods=['POST'])
def analyze():
    try:
        # 搞下上传
        if 'file' in request.files:
            file = request.files['file']
            if file.filename:
                filename = f"upload_{int(time.time())}_{file.filename}"
                input_file = os.path.join(UPLOAD_FOLDER, filename)
                file.save(input_file)
            else:
                return jsonify({'success': False, 'error': '文件名为空'}), 400
        else:
            input_file = request.form.get('input_file', 'input1.txt')
        
        window_size = int(request.form.get('window_size', 600))
        
        task_id = f"task_{int(time.time())}"
        output_file = f"{RESULT_FOLDER}/{task_id}_output.txt"
        
        # 过滤词
        sensitive_words = request.form.get('sensitive_words', '').strip()
        stop_words = request.form.get('stop_words', '').strip()
        
        if sensitive_words:
            sensitive_file = f"{UPLOAD_FOLDER}/sensitive_{task_id}.txt"
            with open(sensitive_file, 'w', encoding='utf-8') as f:
                f.write(sensitive_words)
        
        if stop_words:
            stop_file = f"{UPLOAD_FOLDER}/stop_{task_id}.txt"
            with open(stop_file, 'w', encoding='utf-8') as f:
                f.write(stop_words)
        
        cmd = ['./hotwords', input_file, output_file, str(window_size)]
        
        tasks[task_id] = {
            'status': 'running',
            'input_file': input_file,
            'window_size': window_size,
            'output_file': output_file,
            'start_time': datetime.now().isoformat()
        }
        
        # 跑起来
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        
        if result.returncode == 0:
            with open(output_file, 'r', encoding='utf-8') as f:
                output_content = f.read()
            
            stats = parse_stats(output_content)
            
            tasks[task_id]['status'] = 'completed'
            tasks[task_id]['stats'] = stats
            tasks[task_id]['end_time'] = datetime.now().isoformat()
            
            return jsonify({
                'success': True,
                'task_id': task_id,
                'stats': stats,
                'full_output': output_content,
                'output_file': os.path.basename(output_file)
            })
        else:
            tasks[task_id]['status'] = 'failed'
            tasks[task_id]['error'] = result.stderr
            
            return jsonify({
                'success': False,
                'error': result.stderr or '分析失败'
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({
            'success': False,
            'error': '分析超时'
        }), 500
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/analyze-sample/<sample_name>', methods=['POST'])
def analyze_sample(sample_name):
    try:
        # 检查下名字
        allowed_files = ['input1.txt', 'input2.txt', 'input3.txt', 'input_out_of_order.txt']
        if sample_name not in allowed_files:
            return jsonify({'success': False, 'error': '无效的示例文件'}), 400
        
        if not os.path.exists(sample_name):
            return jsonify({'success': False, 'error': f'文件 {sample_name} 不存在'}), 404
        
        window_size = int(request.form.get('window_size', 600))
        
        task_id = f"task_{int(time.time())}"
        output_file = f"{RESULT_FOLDER}/{task_id}_output.txt"
        
        sensitive_words = request.form.get('sensitive_words', '').strip()
        stop_words = request.form.get('stop_words', '').strip()
        
        if sensitive_words:
            sensitive_file = f"{UPLOAD_FOLDER}/sensitive_{task_id}.txt"
            with open(sensitive_file, 'w', encoding='utf-8') as f:
                f.write(sensitive_words)
        
        if stop_words:
            stop_file = f"{UPLOAD_FOLDER}/stop_{task_id}.txt"
            with open(stop_file, 'w', encoding='utf-8') as f:
                f.write(stop_words)
        
        cmd = ['./hotwords', sample_name, output_file, str(window_size)]
        
        tasks[task_id] = {
            'status': 'running',
            'input_file': sample_name,
            'window_size': window_size,
            'output_file': output_file,
            'start_time': datetime.now().isoformat()
        }
        
        # 跑
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        
        if result.returncode == 0:
            with open(output_file, 'r', encoding='utf-8') as f:
                output_content = f.read()
            
            stats = parse_stats(output_content)
            
            tasks[task_id]['status'] = 'completed'
            tasks[task_id]['stats'] = stats
            tasks[task_id]['end_time'] = datetime.now().isoformat()
            
            return jsonify({
                'success': True,
                'task_id': task_id,
                'stats': stats,
                'full_output': output_content,
                'output_file': os.path.basename(output_file)
            })
        else:
            tasks[task_id]['status'] = 'failed'
            tasks[task_id]['error'] = result.stderr
            
            return jsonify({
                'success': False,
                'error': result.stderr or '分析失败'
            }), 500
            
    except subprocess.TimeoutExpired:
        return jsonify({
            'success': False,
            'error': '分析超时'
        }), 500
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/results/<filename>', methods=['GET'])
def get_result_file(filename):
    filepath = os.path.join(RESULT_FOLDER, filename)
    if os.path.exists(filepath):
        return send_file(filepath, as_attachment=True)
    else:
        return jsonify({'success': False, 'error': '文件不存在'}), 404

@app.route('/api/upload', methods=['POST'])
def upload_file():
    try:
        if 'file' not in request.files:
            return jsonify({'success': False, 'error': '没有文件'}), 400
        
        file = request.files['file']
        if file.filename == '':
            return jsonify({'success': False, 'error': '文件名为空'}), 400
        
        # 存了
        filename = f"upload_{int(time.time())}_{file.filename}"
        filepath = os.path.join(UPLOAD_FOLDER, filename)
        file.save(filepath)
        
        return jsonify({
            'success': True,
            'filename': filename,
            'filepath': filepath
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/tasks/<task_id>', methods=['GET'])
def get_task(task_id):
    if task_id in tasks:
        return jsonify({
            'success': True,
            'task': tasks[task_id]
        })
    else:
        return jsonify({
            'success': False,
            'error': '任务不存在'
        }), 404

@app.route('/api/files', methods=['GET'])
def list_files():
    files = []
    for f in ['input1.txt', 'input2.txt', 'input3.txt']:
        if os.path.exists(f):
            stat = os.stat(f)
            files.append({
                'name': f,
                'size': stat.st_size,
                'lines': count_lines(f)
            })
    
    # 加上上传的
    if os.path.exists(UPLOAD_FOLDER):
        for f in os.listdir(UPLOAD_FOLDER):
            filepath = os.path.join(UPLOAD_FOLDER, f)
            if os.path.isfile(filepath):
                stat = os.stat(filepath)
                files.append({
                    'name': filepath,
                    'size': stat.st_size,
                    'lines': count_lines(filepath)
                })
    
    return jsonify({
        'success': True,
        'files': files
    })

@app.route('/api/download/<task_id>', methods=['GET'])
def download_result(task_id):
    if task_id in tasks and tasks[task_id]['status'] == 'completed':
        output_file = tasks[task_id]['output_file']
        return send_file(output_file, as_attachment=True)
    else:
        return jsonify({'success': False, 'error': '任务不存在或未完成'}), 404

def parse_stats(content):
    stats = {
        'total_lines': 0,
        'queries': 0,
        'unique_words': 0,
        'total_words': 0,
        'windows': 0,
        'avg_query_time': 0,
        'out_of_order': 0
    }
    
    lines = content.split('\n')
    
    for line in lines:
        if '处理的总行数:' in line or '处理的数据行数:' in line:
            try:
                stats['total_lines'] = int(line.split(':')[1].strip())
            except:
                pass
        elif '查询次数:' in line:
            try:
                stats['queries'] = int(line.split(':')[1].strip())
            except:
                pass
        elif '窗口内唯一词数:' in line:
            try:
                stats['unique_words'] = int(line.split(':')[1].strip())
            except:
                pass
        elif '窗口内总词数:' in line:
            try:
                stats['total_words'] = int(line.split(':')[1].strip())
            except:
                pass
        elif '乱序消息数:' in line:
            try:
                parts = line.split(':')[1].strip().split()
                stats['out_of_order'] = int(parts[0])
            except:
                pass
        elif 'Query #' in line:
            stats['queries'] += 1
    
    return stats

def count_lines(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return sum(1 for _ in f)
    except:
        return 0

if __name__ == '__main__':
    print("=" * 50)
    print("  热词统计系统 Web服务器")
    print("  访问地址: http://localhost:5000")
    print("=" * 50)
    app.run(host='0.0.0.0', port=7070, debug=False)
