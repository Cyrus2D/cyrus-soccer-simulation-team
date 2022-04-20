from numpy import array, random
import numpy as np
import multiprocessing
import os


class ReadDataPack:
    def __init__(self):
        self.use_all_data = None
        self.data_label = None
        self.use_pass = None
        self.processes_number = None
        self.pack_number = None
        self.use_cluster = None
        self.counts_file = None
        self.input_data_path = None
        self.train_datas = None
        self.train_labels = None
        self.test_datas = None
        self.test_labels = None

    def get_col_x(self, header_name_to_num):
        cols = []
        # cols.append(['cycle', -1])
        cols.append(['ball_pos_x', -1])
        cols.append(['ball_pos_y', -1])
        cols.append(['ball_pos_r', -1])
        cols.append(['ball_pos_t', -1])
        # cols.append(['ball_kicker_x', -1])
        # cols.append(['ball_kicker_y', -1])
        # cols.append(['ball_kicker_r', -1])
        # cols.append(['ball_kicker_t', -1])
        # if self.use_all_data:
        #     cols.append(['ball_vel_x', -1])
        #     cols.append(['ball_vel_y', -1])
        #     cols.append(['ball_vel_r', -1])
        #     cols.append(['ball_vel_t', -1])
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
            #if use_all_data:
            #    cols.append(['p_l_' + str(p) + '_player_type_dash_rate', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_effort_max', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_effort_min', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_kickable', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_margin', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_kick_power', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_decay', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_size', -1])
            #    cols.append(['p_l_' + str(p) + '_player_type_speed_max', -1])
            cols.append(['p_l_' + str(p) + '_body', -1])
            # cols.append(['p_l_' + str(p) + '_pos_count', -1])
            # if use_all_data:
                # cols.append(['p_l_' + str(p) + '_face', -1])
                # cols.append(['p_l_' + str(p) + '_vel_count', -1])
                # cols.append(['p_l_' + str(p) + '_body_count', -1])
            cols.append(['p_l_' + str(p) + '_pos_x', -1])
            cols.append(['p_l_' + str(p) + '_pos_y', -1])
            cols.append(['p_l_' + str(p) + '_pos_r', -1])
            cols.append(['p_l_' + str(p) + '_pos_t', -1])
            cols.append(['p_l_' + str(p) + '_kicker_x', -1])
            cols.append(['p_l_' + str(p) + '_kicker_y', -1])
            cols.append(['p_l_' + str(p) + '_kicker_r', -1])
            cols.append(['p_l_' + str(p) + '_kicker_t', -1])
            if self.use_all_data:
                cols.append(['p_l_' + str(p) + '_vel_x', -1])
                cols.append(['p_l_' + str(p) + '_vel_y', -1])
                cols.append(['p_l_' + str(p) + '_vel_r', -1])
                cols.append(['p_l_' + str(p) + '_vel_t', -1])
            cols.append(['p_l_' + str(p) + '_in_offside', -1])
            cols.append(['p_l_' + str(p) + '_is_kicker', -1])
            cols.append(['p_l_' + str(p) + '_is_ghost', -1])
            cols.append(['p_l_' + str(p) + '_pass_dist', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp1_dist', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp1_dist_proj_to_opp', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp1_dist_proj_to_kicker', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp1_open_angle', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp1_dist_diffbody', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp2_dist', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp2_dist_proj_to_opp', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp2_dist_proj_to_kicker', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp2_open_angle', -1])
            cols.append(['p_l_' + str(p) + '_pass_opp2_dist_diffbody', -1])
            if self.use_all_data:
                cols.append(['p_l_' + str(p) + '_pass_opp3_dist', -1])
                cols.append(['p_l_' + str(p) + '_pass_opp3_dist_proj_to_opp', -1])
                cols.append(['p_l_' + str(p) + '_pass_opp3_dist_proj_to_kicker', -1])
                cols.append(['p_l_' + str(p) + '_pass_opp3_open_angle', -1])
                cols.append(['p_l_' + str(p) + '_pass_opp3_dist_diffbody', -1])

            # if use_all_data:
            cols.append(['p_l_' + str(p) + '_near1_opp_dist', -1])
            cols.append(['p_l_' + str(p) + '_near1_opp_angle', -1])
            cols.append(['p_l_' + str(p) + '_near1_opp_diffbody', -1])
            cols.append(['p_l_' + str(p) + '_near2_opp_dist', -1])
            cols.append(['p_l_' + str(p) + '_near2_opp_angle', -1])
            cols.append(['p_l_' + str(p) + '_near2_opp_diffbody', -1])
            if self.use_all_data:
                cols.append(['p_l_' + str(p) + '_angle_goal_center_r', -1])
                cols.append(['p_l_' + str(p) + '_angle_goal_center_t', -1])
            cols.append(['p_l_' + str(p) + '_open_goal_angle', -1])

        for p in range(1, 16):
            cols.append(['p_r_' + str(p) + '_unum', -1])
            #if use_all_data:
            #    cols.append(['p_r_' + str(p) + '_player_type_dash_rate', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_effort_max', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_effort_min', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_kickable', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_margin', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_kick_power', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_decay', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_size', -1])
            #    cols.append(['p_r_' + str(p) + '_player_type_speed_max', -1])
            cols.append(['p_r_' + str(p) + '_body', -1])
            # cols.append(['p_r_' + str(p) + '_pos_count', -1])
            # if use_all_data:
                # cols.append(['p_r_' + str(p) + '_face', -1])
                # cols.append(['p_r_' + str(p) + '_vel_count', -1])
                # cols.append(['p_r_' + str(p) + '_body_count', -1])
            cols.append(['p_r_' + str(p) + '_pos_x', -1])
            cols.append(['p_r_' + str(p) + '_pos_y', -1])
            cols.append(['p_r_' + str(p) + '_pos_r', -1])
            cols.append(['p_r_' + str(p) + '_pos_t', -1])
            cols.append(['p_r_' + str(p) + '_kicker_x', -1])
            cols.append(['p_r_' + str(p) + '_kicker_y', -1])
            cols.append(['p_r_' + str(p) + '_kicker_r', -1])
            cols.append(['p_r_' + str(p) + '_kicker_t', -1])
            if self.use_all_data:
                cols.append(['p_r_' + str(p) + '_vel_x', -1])
                cols.append(['p_r_' + str(p) + '_vel_y', -1])
                cols.append(['p_r_' + str(p) + '_vel_r', -1])
                cols.append(['p_r_' + str(p) + '_vel_t', -1])
            # cols.append(['p_r_' + str(p) + '_is_ghost', -1])

        for c in range(len(cols)):
            cols[c][1] = header_name_to_num[cols[c][0]]
            cols[c] = cols[c][1]
        return cols


    def get_col_y(self, header_name_to_num):
        cols = []
        # cols.append('out_category')
        # cols.append('out_targetx')
        # cols.append('out_targety')
        if self.data_label == 'index':
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

    def read_file(self, file_path):
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
            row = line.split(',')[:]
            if len(row) != len(header):
                print('error in line', line_number, len(row))
                continue
            f_row = []
            for r in row:
                f_row.append(float(r))
            if self.use_pass:
                if f_row[out_cat_number] >= 2.0:
                    rows.append(f_row)
            else:
                rows.append(f_row)
        return header_name_to_num, rows

    def read_files_pack(self, files):
        a_pool = multiprocessing.Pool(processes=self.processes_number)
        result = a_pool.map(self.read_file, files)
        header = result[0][0]
        rows = []
        for r in result:
            rows += r[1]

        return header, rows

    def read_files(self, path):
        l = os.listdir(path)
        print(path)
        print('file_numbers', len(l))
        files = []
        f_number = 0
        file_counts = len(l) if not self.counts_file else self.counts_file
        print(file_counts)
        for f in l[:file_counts]:
            if f.endswith('csv'):
                files.append([os.path.join(path, f), f_number])
                f_number += 1
        print(f_number)
        all_data_x = None
        all_data_y = None
        for p in range(self.pack_number):
            header_name_to_num, rows = self.read_files_pack(
                files[int(f_number / self.pack_number * p): int(f_number / self.pack_number * (p + 1))])
            all_data = array(rows)
            del rows
            cols = self.get_col_x(header_name_to_num)
            array_cols = array(cols)
            del cols
            data_x = all_data[:, array_cols[:]]

            cols_numb_y = self.get_col_y(header_name_to_num)
            array_cols_numb_y = array(cols_numb_y)
            del cols_numb_y
            data_y = (all_data[:, array_cols_numb_y[:]])
            if not self.use_cluster:
                data_y[:, 0] /= 3.0
                data_y[:, 1] += 180.0
                data_y[:, 1] /= 360.0
            del all_data
            if all_data_x is None:
                all_data_x = data_x
            else:
                all_data_x = np.concatenate((all_data_x, data_x), axis=0)
            if all_data_y is None:
                all_data_y = data_y
            else:
                all_data_y = np.concatenate((all_data_y, data_y), axis=0)
            print(f'{(p + 1) / self.pack_number * 100}% processed', all_data_x.shape, all_data_y.shape, data_x.shape,
                  data_y.shape)
        return all_data_x, all_data_y

    def read_and_separate_data(self, use_all_data):
        if use_all_data == self.use_all_data:
            return
        else:
            self.use_all_data = use_all_data
            self.train_datas = None
            self.train_labels = None
            self.test_datas = None
            self.test_labels = None
        print('#'*100, 'start reading')
        data_x, data_y = self.read_files(self.input_data_path)
        data_size = data_x.shape[0]
        train_size = int(data_size * 0.8)

        randomize = np.arange(data_size)
        np.random.shuffle(randomize)
        X = data_x[randomize]
        del data_x
        Y = data_y[randomize]
        del data_y
        self.train_datas = X[:train_size]
        self.train_labels = Y[:train_size]
        self.test_datas = X[train_size + 1:]
        self.test_labels = Y[train_size + 1:]
        del X
        del Y
