from keras import layers, models, activations, losses, metrics, optimizers
import matplotlib.pyplot as plt
from numpy import array, random
import numpy as np
from keras.utils import to_categorical
import keras.backend as K
from analyzer.read_files import read_folder_multi_thread
import tensorflow as tf
from keras import regularizers
import sys

k_best = 1
use_pass = False
use_cluster = True

data_x, data_y = read_folder_multi_thread("/mnt/f/xxx")
data_x = array(data_x)
data_y = array(data_y)

data_size = data_x.shape[0]
train_size = int(data_size*0.8)

print("SHAPE:")
print(data_y[0].shape)


randomize = np.arange(data_size)
np.random.shuffle(randomize)
X = data_x[randomize]
del data_x
Y = data_y[randomize]
del data_y
train_datas = X[:train_size]
train_labels = Y[:train_size]
test_datas = X[train_size + 1:]
test_labels = Y[train_size + 1:]
del X
del Y


if use_cluster:
    # train_labels = to_categorical(train_labels)
    # test_labels = to_categorical(test_labels)
    print(train_datas.shape, train_labels.shape)
    print(test_datas.shape, test_labels.shape)

    network = models.Sequential()
    network.add(layers.Dense(512, activation=activations.relu, input_shape=(train_datas[0].shape[0],)))
    network.add(layers.Dense(256, activation=activations.relu))
    network.add(layers.Dense(64, activation=activations.relu))
    network.add(
        layers.Dense(train_labels[0].shape[0], activation=activations.softmax))

    def accuracy(y_true, y_pred):
        y_true = K.cast(y_true, y_pred.dtype)
        y_true = K.argmax(y_true)
        # y_pred1 = K.argmax(y_pred)
        res = K.in_top_k(y_pred, y_true, k_best)
        return res

    network.compile(optimizer=optimizers.Adam(learning_rate=0.0002), loss=losses.categorical_crossentropy, metrics=[accuracy])
    history = network.fit(train_datas, train_labels, epochs=50, batch_size=64, validation_data=(test_datas, test_labels))
    res = network.predict(test_datas)
    for i in range(len(test_datas)):
        print(test_labels[i], res[i])
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
    plt.savefig(run_name)
    network.save(run_name + '.h5')
    file = open('history_' + run_name, 'w')
    file.write(str(history_dict))
    file.close()
    file = open('best_' + run_name, 'w')
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
