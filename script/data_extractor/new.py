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


setting_number = int(sys.argv[1])

settings = [
    ['d1', 'imp_data', 'index'],
    ['d2', 'imp_data', 'index'],
    ['d3', 'imp_data', 'index'],
    ['d4', 'imp_data', 'index'],
    ['d5', 'imp_data', 'index'],
    ['d6', 'imp_data', 'index'],
    ['d7', 'imp_data', 'index'],
    ['d8', 'imp_data', 'index'],
    ['d1', 'all_data', 'index'],
    ['d2', 'all_data', 'index'],
    ['d3', 'all_data', 'index'],
    ['d4', 'all_data', 'index'],
    ['d5', 'all_data', 'index'],
    ['d6', 'all_data', 'index'],
    ['d7', 'all_data', 'index'],
    ['d8', 'all_data', 'index'],
    ['d1', 'imp_data', 'unum'],
    ['d2', 'imp_data', 'unum'],
    ['d3', 'imp_data', 'unum'],
    ['d4', 'imp_data', 'unum'],
    ['d5', 'imp_data', 'unum'],
    ['d6', 'imp_data', 'unum'],
    ['d7', 'imp_data', 'unum'],
    ['d8', 'imp_data', 'unum'],
    ['d1', 'all_data', 'unum'],
    ['d2', 'all_data', 'unum'],
    ['d3', 'all_data', 'unum'],
    ['d4', 'all_data', 'unum'],
    ['d5', 'all_data', 'unum'],
    ['d6', 'all_data', 'unum'],
    ['d7', 'all_data', 'unum'],
    ['d8', 'all_data', 'unum'],
    ['d7', 'imp_data', 'index']
]
setting = ['d9', 'imp_data', 'index']#settings[setting_number]
print(setting)
data_name = setting[0]
input_data_path = f'/data1/aref/2d/{data_name}/'
output_path = f'./res/{setting_number}'
run_name = f'{setting[0]}-{setting[1]}-{setting[2]}'
pathlib.Path(output_path).mkdir(parents=True, exist_ok=True)
data_label = setting[2]
use_all_data = True if setting[1] == 'all_data' else False
k_best = 1
use_pass = False
print(run_name)
use_cluster = True


def get_col_x(header_name_to_num):
    cols = []
    # cols.append(['cycle', -1])
    cols.append(['ball_pos_x', -1])
    cols.append(['ball_pos_y', -1])
    cols.append(['ball_pos_r', -1])
    cols.append(['ball_pos_t', -1])
    cols.append(['ball_kicker_x', -1])
    cols.append(['ball_kicker_y', -1])
    cols.append(['ball_kicker_r', -1])
    cols.append(['ball_kicker_t', -1])
    if use_all_data:
        cols.append(['ball_vel_x', -1])
        cols.append(['ball_vel_y', -1])
        cols.append(['ball_vel_r', -1])
        cols.append(['ball_vel_t', -1])
    cols.append(['offside_count', -1])
    # for i in range(12):
    #     cols.append(['drible_angle_0', -1])
    #     cols.append(['drible_angle_1', -1])
    #     cols.append(['drible_angle_2', -1])
    #     cols.append(['drible_angle_3', -1])
    #     cols.append(['drible_angle_4', -1])
    #     cols.append(['drible_angle_5', -1])
    #     cols.append(['drible_angle_6', -1])
    #     cols.append(['drible_angle_7', -1])
    #     cols.append(['drible_angle_8', -1])
    #     cols.append(['drible_angle_9', -1])
    #     cols.append(['drible_angle_10', -1])
    #     cols.append(['drible_angle_11', -1])

    for p in range(1, 12):
        cols.append(['p_l_' + str(p) + '_unum', -1])
        if use_all_data:
            cols.append(['p_l_' + str(p) + '_player_type_dash_rate', -1])
            cols.append(['p_l_' + str(p) + '_player_type_effort_max', -1])
            cols.append(['p_l_' + str(p) + '_player_type_effort_min', -1])
            cols.append(['p_l_' + str(p) + '_player_type_kickable', -1])
            cols.append(['p_l_' + str(p) + '_player_type_margin', -1])
            cols.append(['p_l_' + str(p) + '_player_type_kick_power', -1])
            cols.append(['p_l_' + str(p) + '_player_type_decay', -1])
            cols.append(['p_l_' + str(p) + '_player_type_size', -1])
            cols.append(['p_l_' + str(p) + '_player_type_speed_max', -1])
        cols.append(['p_l_' + str(p) + '_body', -1])
        cols.append(['p_l_' + str(p) + '_pos_count', -1])
        if use_all_data:
            cols.append(['p_l_' + str(p) + '_face', -1])
            cols.append(['p_l_' + str(p) + '_vel_count', -1])
            cols.append(['p_l_' + str(p) + '_body_count', -1])
        cols.append(['p_l_' + str(p) + '_pos_x', -1])
        cols.append(['p_l_' + str(p) + '_pos_y', -1])
        cols.append(['p_l_' + str(p) + '_pos_r', -1])
        cols.append(['p_l_' + str(p) + '_pos_t', -1])
        cols.append(['p_l_' + str(p) + '_kicker_x', -1])
        cols.append(['p_l_' + str(p) + '_kicker_y', -1])
        cols.append(['p_l_' + str(p) + '_kicker_r', -1])
        cols.append(['p_l_' + str(p) + '_kicker_t', -1])
        if use_all_data:
            cols.append(['p_l_' + str(p) + '_vel_x', -1])
            cols.append(['p_l_' + str(p) + '_vel_y', -1])
            cols.append(['p_l_' + str(p) + '_vel_r', -1])
            cols.append(['p_l_' + str(p) + '_vel_t', -1])
        cols.append(['p_l_' + str(p) + '_in_offside', -1])
        cols.append(['p_l_' + str(p) + '_is_kicker', -1])
        cols.append(['p_l_' + str(p) + '_is_ghost', -1])
        cols.append(['p_l_' + str(p) + '_pass_dist', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp1_dist', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp1_angle', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp1_dist_line', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp1_dist_proj', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp1_dist_diffbody', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp2_dist', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp2_angle', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp2_dist_line', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp2_dist_proj', -1])
        cols.append(['p_l_' + str(p) + '_pass_opp2_dist_diffbody', -1])
        #if use_all_data:
        cols.append(['p_l_' + str(p) + '_near1_opp_dist', -1])
        cols.append(['p_l_' + str(p) + '_near1_opp_angle', -1])
        cols.append(['p_l_' + str(p) + '_near1_opp_diffbody', -1])
        cols.append(['p_l_' + str(p) + '_near2_opp_dist', -1])
        cols.append(['p_l_' + str(p) + '_near2_opp_angle', -1])
        cols.append(['p_l_' + str(p) + '_near2_opp_diffbody', -1])
        cols.append(['p_l_' + str(p) + '_angle_goal_center_r', -1])
        cols.append(['p_l_' + str(p) + '_angle_goal_center_t', -1])

    for p in range(1, 16):
        cols.append(['p_r_' + str(p) + '_unum', -1])
        if use_all_data:
            cols.append(['p_r_' + str(p) + '_player_type_dash_rate', -1])
            cols.append(['p_r_' + str(p) + '_player_type_effort_max', -1])
            cols.append(['p_r_' + str(p) + '_player_type_effort_min', -1])
            cols.append(['p_r_' + str(p) + '_player_type_kickable', -1])
            cols.append(['p_r_' + str(p) + '_player_type_margin', -1])
            cols.append(['p_r_' + str(p) + '_player_type_kick_power', -1])
            cols.append(['p_r_' + str(p) + '_player_type_decay', -1])
            cols.append(['p_r_' + str(p) + '_player_type_size', -1])
            cols.append(['p_r_' + str(p) + '_player_type_speed_max', -1])
        cols.append(['p_r_' + str(p) + '_body', -1])
        cols.append(['p_r_' + str(p) + '_pos_count', -1])
        if use_all_data:
            cols.append(['p_r_' + str(p) + '_face', -1])
            cols.append(['p_r_' + str(p) + '_vel_count', -1])
            cols.append(['p_r_' + str(p) + '_body_count', -1])
        cols.append(['p_r_' + str(p) + '_pos_x', -1])
        cols.append(['p_r_' + str(p) + '_pos_y', -1])
        cols.append(['p_r_' + str(p) + '_pos_r', -1])
        cols.append(['p_r_' + str(p) + '_pos_t', -1])
        cols.append(['p_r_' + str(p) + '_kicker_x', -1])
        cols.append(['p_r_' + str(p) + '_kicker_y', -1])
        cols.append(['p_r_' + str(p) + '_kicker_r', -1])
        cols.append(['p_r_' + str(p) + '_kicker_t', -1])
        if use_all_data:
            cols.append(['p_r_' + str(p) + '_vel_x', -1])
            cols.append(['p_r_' + str(p) + '_vel_y', -1])
            cols.append(['p_r_' + str(p) + '_vel_r', -1])
            cols.append(['p_r_' + str(p) + '_vel_t', -1])
        #cols.append(['p_r_' + str(p) + '_is_ghost', -1])

    for c in range(len(cols)):
        cols[c][1] = header_name_to_num[cols[c][0]]
        cols[c] = cols[c][1]
    return cols


def get_col_y(header_name_to_num):
    cols = []
    # cols.append('out_category')
    # cols.append('out_targetx')
    # cols.append('out_targety')
    if data_label == 'index':
        cols.append('out_unum_index')
    else:
        cols.append('out_unum')
    # cols.append("out_ball_speed")
    # cols.append(" out_ball_dir")
    # cols.append("out_desc")

    cols_numb = []
    for c in range(len(cols)):
        cols_numb.append(header_name_to_num[cols[c]])
    return cols_numb


def read_file(file_path):
    file = open(file_path[0], 'r')
    lines = file.readlines()[:]
    header = lines[0].split(',')[:-1]
    header_name_to_num = {}
    out_cat_number = 0
    counter = 0
    for h in header:
        header_name_to_num[h] = counter
        if h == 'out_category':
            out_cat_number = counter
        counter += 1
    rows = []
    line_number = 0
    for line in lines[1:]:
        line_number += 1
        row = line.split(',')[:-1]
        if len(row) != len(header):
            print('error in line', line_number, len(row))
            continue
        f_row = []
        for r in row:
            f_row.append(float(r))
        if use_pass:
            if f_row[out_cat_number] >= 2.0:
                rows.append(f_row)
        else:
            rows.append(f_row)
    print(file_path[1])
    return header_name_to_num, rows


def read_files(path):
    l = os.listdir(path)
    files = []
    f_number = 0
    for f in l:
        if f.endswith('csv'):
            files.append([os.path.join(path, f), f_number])
            f_number+=1
    a_pool = multiprocessing.Pool(processes=100)
    result = a_pool.map(read_file, files)
    header = result[0][0]
    rows = []
    for r in result:
        rows += r[1]
    # print(header)
    # print(len(rows))
    # f = open(path + 'all.csv', 'w')
    # f.write(','.join(header)+'\n')
    # for r in rows:
    #     for v in r:
    #         f.write(str(v) + ',')
    #     f.write('\n')

    return header, rows


header_name_to_num, rows = read_files(input_data_path)

all_data = array(rows)
cols = get_col_x(header_name_to_num)
array_cols = array(cols)
del cols
print(array_cols.shape)
print(all_data.shape)
data_x = all_data[:, array_cols[:]]

cols_numb_y = get_col_y(header_name_to_num)
array_cols_numb_y = array(cols_numb_y)
del cols_numb_y
data_y = (all_data[:, array_cols_numb_y[:]])
# data_y[:, 0] +
if not use_cluster:
    data_y[:, 0] /= 3.0
    data_y[:, 1] += 180.0
    data_y[:, 1] /= 360.0
del all_data
data_size = data_x.shape[0]
train_size = int(data_size * 0.8)

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
    train_labels = to_categorical(train_labels, num_classes=12)
    test_labels = to_categorical(test_labels, num_classes=12)
    print(train_datas.shape, train_labels.shape)
    print(test_datas.shape, test_labels.shape)

    network = models.Sequential()
    network.add(layers.Dense(350, activation=activations.relu, input_shape=(train_datas.shape[1],)))
    #network.add(layers.Dense(256, activation=activations.relu))
    network.add(layers.Dense(250, activation=activations.relu))
    network.add(
        layers.Dense(train_labels.shape[1], activation=activations.softmax))


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
        keras.callbacks.ModelCheckpoint(filepath=os.path.join(output_path,'-best_model.{epoch:02d}-{val_accuracy:.3f}-{val_accuracy2:.3f}.h5'), save_best_only=True, monitor='val_accuracy', mode='max'),
        keras.callbacks.TensorBoard(log_dir=output_path)
    ]
    network.compile(optimizer=optimizers.Adam(learning_rate=0.0001), loss=losses.categorical_crossentropy,
                    metrics=[accuracy, accuracy2])
    history = network.fit(train_datas, train_labels, epochs=100, batch_size=64, callbacks=my_call_back,
                          validation_data=(test_datas, test_labels))
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
