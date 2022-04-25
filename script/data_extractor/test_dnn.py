
import keras.callbacks
from keras import layers, models, activations, losses, metrics, optimizers
import matplotlib.pyplot as plt
from numpy import array, random
import numpy as np
from keras.utils import to_categorical
import keras.backend as K
import tensorflow as tf
from keras import regularizers
import sys
import multiprocessing
import os
import pathlib


root_out = 'res'
setting_number = -1 if len(sys.argv) == 1 else int(sys.argv[1])

settings = [
    ['pass_pred_yushan', 'imp_data', 'index', 2, 64],
    # ['pass_pred_yushan', 'all_data', 'index', 2, 64],
]
from read_data_pack import ReadDataPack
rdp = ReadDataPack()
rdp.data_label = 'index'
rdp.use_pass = False
rdp.processes_number = 1
rdp.pack_number = 1
rdp.use_cluster = True
rdp.counts_file = None
rdp.input_data_path = f'data/'

for setting_number in range(len(settings)):
    setting = settings[setting_number]
    print(setting)
    data_name = setting[0]
    output_path = f'./{root_out}/{setting_number}'
    run_name = f'{setting[0]}-{setting[1]}-{setting[2]}'
    pathlib.Path(output_path).mkdir(parents=True, exist_ok=True)
    data_label = setting[2]
    use_all_data = True if setting[1] == 'all_data' else False
    k_best = 1
    print(run_name)
    use_cluster = True
    epochs = 300
    batch_size = setting[4]
    rdp.read_and_separate_data(use_all_data)
    train_datas = rdp.train_datas
    train_labels = rdp.train_labels
    test_datas = rdp.test_datas
    test_labels = rdp.test_labels
    train_labels = to_categorical(train_labels, num_classes=12)
    test_labels = to_categorical(test_labels, num_classes=12)
    print(train_datas.shape, train_labels.shape)
    print(test_datas.shape, test_labels.shape)
    def accuracy(y_true, y_pred):
        y_true = K.cast(y_true, y_pred.dtype)
        y_true = K.argmax(y_true)
        # y_pred1 = K.argmax(y_pred)
        res = K.in_top_k(y_pred, y_true, k_best)
        return res


    def accuracy2(y_true, y_pred):
        y_true = K.cast(y_true, y_pred.dtype)
        y_true = K.argmax(y_true)
        # y_pred1 = K.argmax(y_pred)
        res = K.in_top_k(y_pred, y_true, 2)
        return res
    network = keras.models.load_model('../../team/src/data/deep/pass_prediction_yushan_w_w.h5', custom_objects={"accuracy": accuracy, "accuracy2": accuracy2})

    my_call_back = [
        keras.callbacks.ModelCheckpoint(
            filepath=output_path + '/' + run_name + '-best_model.{epoch:02d}-{val_accuracy:.3f}-{val_accuracy2:.3f}.h5',
            save_best_only=True, monitor='val_accuracy', mode='max'),
        keras.callbacks.TensorBoard(log_dir=output_path)
    ]
    network.compile(optimizer=optimizers.Adam(learning_rate=0.0001), loss=losses.categorical_crossentropy,
                    metrics=[accuracy, accuracy2])
    print(train_datas.shape, train_labels.shape)
    print(test_datas.shape, test_labels.shape)
    res = network.predict(test_datas)
    print(res.shape)
    correct = 0
    for i in range(res.shape[0]):
        p_res = np.where(res[i] == np.amax(res[i]))
        r_res = np.where(test_labels[i] == np.amax(test_labels[i]))
        print(test_datas[i], res[i], np.amax(res[i]), test_labels[i], p_res, r_res)
        if p_res[0] == r_res[0]:
            correct += 1
        s = ""
        for x in range(test_datas.shape[1]):
            s += str(test_datas[i][x]) + ','
        print(s)
        exit()
    print(correct / res.shape[0] * 100)
