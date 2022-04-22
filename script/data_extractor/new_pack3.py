
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
    ['pass_pred_yushan', 'all_data', 'index', 2, 64],
]
from read_data_pack import ReadDataPack
rdp = ReadDataPack()
rdp.data_label = 'index'
rdp.use_pass = False
rdp.processes_number = 100
rdp.pack_number = 20
rdp.use_cluster = True
rdp.counts_file = None
rdp.input_data_path = f'/data1/nader/workspace/robo/pass_pred_yushan/'

for setting_number in range(len(settings)):
    setting = settings[setting_number]
    print(setting)
    data_name = setting[0]
    output_path = f'./{root_out}/{setting_number}'
    run_name = f'{setting[0]}-{setting[1]}-{setting[2]}'
    pathlib.Path(output_path).mkdir(parents=True, exist_ok=True)
    data_label = setting[2]
    dnn_sizes = {
        0: 'very_big',
        1: 'big',
        2: 'normal',
        3: 'small',
        4: 'very_small',
        5: 'to_small_to_big',
    }
    use_all_data = True if setting[1] == 'all_data' else False
    dnn_size = dnn_sizes[setting[3]]
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

    if use_cluster:
        train_labels = to_categorical(train_labels, num_classes=12)
        test_labels = to_categorical(test_labels, num_classes=12)
        print(train_datas.shape, train_labels.shape)
        print(test_datas.shape, test_labels.shape)
        network = models.Sequential()
        if dnn_size == 'very_big':
            network.add(layers.Dense(512, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(256, activation=activations.relu))
            network.add(layers.Dense(128, activation=activations.relu))
            network.add(layers.Dense(64, activation=activations.relu))
        elif dnn_size == 'big':
            network.add(layers.Dense(350, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(250, activation=activations.relu))
        elif dnn_size == 'normal':
            network.add(layers.Dense(128, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(64, activation=activations.relu))
            network.add(layers.Dense(32, activation=activations.relu))
        elif dnn_size == 'small':
            network.add(layers.Dense(64, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(32, activation=activations.relu))
        elif dnn_size == 'very_small':
            network.add(layers.Dense(32, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(16, activation=activations.relu))
        elif dnn_size == 'to_small_to_big':
            network.add(layers.Dense(64, activation=activations.relu, input_shape=(train_datas.shape[1],)))
            network.add(layers.Dense(128, activation=activations.relu))
            network.add(layers.Dense(64, activation=activations.relu))
        network.add(layers.Dense(train_labels.shape[1], activation=activations.softmax))

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

        history = network.fit(train_datas, train_labels, epochs=epochs, batch_size=batch_size, callbacks=my_call_back,
                              validation_data=(test_datas, test_labels))
        history_dict = history.history
        print(history_dict)
        loss_values = history_dict['loss']
        val_loss_values = history_dict['val_loss']
        acc_values = history_dict['accuracy']
        val_acc_values = history_dict['val_accuracy']
        epochs = range(len(loss_values))
        plt.figure(1)
        plt.subplot(211)
        plt.plot(epochs, loss_values, 'r--', label='Training loss')
        plt.plot(epochs, val_loss_values, 'b--', label='Validation loss')
        plt.title("train/test loss")
        plt.xlabel('Epochs')
        plt.ylabel('Loss')
        plt.legend()
        plt.subplot(212)
        plt.plot(epochs, acc_values, 'r--', label='Training mean_squared_error')
        plt.plot(epochs, val_acc_values, '--', label='Validation mean_squared_error')
        plt.title("train/test acc")
        plt.xlabel('Epochs')
        plt.ylabel('Acc')
        plt.legend()
        plt.savefig(os.path.join(output_path, 'fig_' + run_name + '.png'))
        network.save(os.path.join(output_path, 'net_' + run_name + '.h5'))
        file = open(os.path.join(output_path, 'history_' + run_name), 'w')
        file.write(str(history_dict))
        file.close()
        file = open(os.path.join(output_path, 'best_' + run_name), 'w')
        file.write(str(max(val_acc_values)))
        file.close()

    else:
        print(train_datas.shape, train_labels.shape)
        print(test_datas.shape, test_labels.shape)
        network = models.Sequential()
        network.add(layers.Dense(512, activation=activations.relu, input_shape=(train_datas.shape[1],)))
        network.add(layers.Dense(256, activation=activations.relu))
        network.add(layers.Dense(64, activation=activations.relu))
        network.add(layers.Dense(train_labels.shape[1], activation=activations.sigmoid))

        network.compile(optimizer=optimizers.Adam(learning_rate=0.001), loss=losses.mse, metrics=[metrics.mse])
        history = network.fit(train_datas, train_labels, epochs=100, batch_size=32,
                              validation_data=(test_datas, test_labels))

        history_dict = history.history

        loss_values = history_dict['loss']
        val_loss_values = history_dict['val_loss']
        acc_values = history_dict['mean_squared_error']
        val_acc_values = history_dict['val_mean_squared_error']

        epochs = range(len(loss_values))
        plt.figure(1)
        plt.subplot(211)
        plt.plot(epochs, loss_values, 'r--', label='Training loss')
        plt.plot(epochs, val_loss_values, 'b--', label='Validation loss')
        plt.title("train/test loss")
        plt.xlabel('Epochs')
        plt.ylabel('Loss')
        plt.legend()
        plt.subplot(212)
        plt.plot(epochs, acc_values, 'r--', label='Training mean_squared_error')
        plt.plot(epochs, val_acc_values, '--', label='Validation mean_squared_error')
        plt.title("train/test acc")
        plt.xlabel('Epochs')
        plt.ylabel('Acc')
        plt.legend()
        plt.savefig(run_name)
        network.save(run_name + '.h5')
        file = open('history_' + run_name, 'w')
        file.write(str(history_dict))
        file.close()
        file = open('best_' + run_name, 'w')
        file.write(str(max(val_acc_values)))
        file.close()
