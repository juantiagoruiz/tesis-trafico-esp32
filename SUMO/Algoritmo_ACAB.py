# Algoritmo de ACAB

# Paso 1: Agregar modulos para proveer acceso a librerias y funciones especificas
import os # Este modulo provee funciones para manejar filepaths, directories, environment variables
import sys # Este modulo provee acceso a parametros y funciones especificos de python
import random
import numpy as np
import matplotlib.pyplot as plt  # Visualizacion
import pandas as pd

# Paso 2: Establece path a SUMO (SUMO_HOME)
if 'SUMO_HOME' in os.environ:
    tools = os.path.join(os.environ['SUMO_HOME'], 'tools')
    sys.path.append(tools)
else:
    sys.exit("Please declare environment variable 'SUMO_HOME'")

# Paso 3: Agrega modulo TRACI
import traci # Informacion estatica de red

# Paso 4: Define configuracion de Sumo
Sumo_config = [
    'sumo-gui',
    '-c', 'osm_actuated.sumocfg',
    '--step-length', '0.05',
    '--delay', '1000',
    '--lateral-resolution', '0.1',
    '--duration-log.statistics'
]

# Paso 5: Abre conexion entre SUMO y Traci
traci.start(Sumo_config)

# Paso 6: Define variables
vehicle_speed = 0
total_speed = 0
total_vehicles = 0
#global current_vehicles
current_vehicles = 0
step_count = 0
tiempo_seteado_TFL1 = 0
tiempo_seteado_TFL2 = 0
tiempo_seteado_TFL3 = 0
tiempo_seteado_TFL4 = 0
waiting_time = 0
emissions = 0
#total_vehicles_EO = 0
#total_vehicles_NS = 0
#total_vehicles_SN = 0

# Variable para almacenar el tiempo de partida de cada vehiculo cuando aparecen por primera vez en la simulacion
depart_times = {}
# Variable para almacenar el tiempo de viaje calculado par vehiculos despues de que llegan a destino
travel_times = {}

# Paso 7: Define funciones
def set_TFL(TFL,E2_EO1,E2_EO2,E2_EO3,E2_NS,E2_SN,lanes_EO,lanes_NS,tiempo_seteado):
    total_vehicles_EO = 0
    total_vehicles_NS = 0
    total_vehicles_SN = 0
    if traci.trafficlight.getPhase(TFL) == 1 or traci.trafficlight.getPhase(TFL) == 3 or traci.trafficlight.getPhase(TFL) == 5:
        tiempo_seteado = 0
    
    if tiempo_seteado == 0:
    
        if traci.lanearea.getLastIntervalVehicleNumber(E2_EO1) > 0 or traci.lanearea.getLastIntervalVehicleNumber(E2_EO2) > 0 or traci.lanearea.getLastIntervalVehicleNumber(E2_EO3) > 0:
            total_vehicles_EO = traci.lanearea.getLastIntervalVehicleNumber(E2_EO1)+traci.lanearea.getLastIntervalVehicleNumber(E2_EO2)+traci.lanearea.getLastIntervalVehicleNumber(E2_EO3)
            #print(total_vehicles)
        if traci.lanearea.getLastIntervalVehicleNumber(E2_NS) > 0:
            total_vehicles_NS = traci.lanearea.getLastIntervalVehicleNumber(E2_NS)
            #print(total_vehicles)
        if traci.lanearea.getLastIntervalVehicleNumber(E2_SN) > 0:
            total_vehicles_SN = traci.lanearea.getLastIntervalVehicleNumber(E2_SN)
            #print(total_vehicles)
    
        if traci.trafficlight.getPhase(TFL) == 0:            
            traci.trafficlight.setPhaseDuration(TFL,(3+(total_vehicles_NS)/lanes_NS))
            print(total_vehicles_NS)
            tiempo_seteado = 1
        
        if traci.trafficlight.getPhase(TFL) == 2:            
            traci.trafficlight.setPhaseDuration(TFL,(3+(total_vehicles_SN)/lanes_NS))
            print(total_vehicles_SN)
            tiempo_seteado = 1
    
        if traci.trafficlight.getPhase(TFL) == 4:            
            traci.trafficlight.setPhaseDuration(TFL,(3+(total_vehicles_EO)/lanes_EO))
            print(total_vehicles_EO)
            tiempo_seteado = 1
        return tiempo_seteado

def get_state():  # Obtiene estado actual de la simulacion
    global q_EB_0, q_EB_1, q_EB_2, q_SB_0, q_SB_1, q_SB_2, current_phase
    
    # IDs de deteccion para Node1
    detector_Node1a = "e2_0"
    detector_Node1b = "e2_1"
    detector_Node1c = "e2_2"
    detector_Node1d = "e2_15"
    detector_Node1e = "e2_16"
    
    # IDs de deteccion para Node2
    detector_Node2a = "e2_3"
    detector_Node2b = "e2_4"
    detector_Node2c = "e2_5"
    detector_Node2d = "e2_13"
    detector_Node2e = "e2_14"

    # IDs de deteccion para Node3
    detector_Node3a = "e2_6"
    detector_Node3b = "e2_7"
    detector_Node3c = "e2_8"
    detector_Node3d = "e2_9"
    detector_Node3e = "e2_10"
    detector_Node3f = "e2_11"
    detector_Node3g = "e2_12"

    # IDs de deteccion para Node4
    detector_Node4a = "e2_21"
    detector_Node4b = "e2_22"
    detector_Node4c = "e2_23"
    detector_Node4d = "e2_24"
    detector_Node4e = "e2_25"
    detector_Node4f = "e2_26"
    detector_Node4g = "e2_27"

    # IDs de deteccion para Node5
    detector_Node5a = "e2_17"
    detector_Node5b = "e2_18"
    detector_Node5c = "e2_19"
    detector_Node5d = "e2_20"
    detector_Node5e = "e2_28"
    
    # ID de luz de trafico
    traffic_light_id = "1738355191"
    
    # Obtiene longitud de cola para cada detector
    q_1a = get_queue_length(detector_Node1a)
    q_1b = get_queue_length(detector_Node1b)
    q_1c = get_queue_length(detector_Node1c)
    q_1d = get_queue_length(detector_Node1d)
    q_1e = get_queue_length(detector_Node1e)
    
    q_2a = get_queue_length(detector_Node2a)
    q_2b = get_queue_length(detector_Node2b)
    q_2c = get_queue_length(detector_Node2c)
    q_2d = get_queue_length(detector_Node2d)
    q_2e = get_queue_length(detector_Node2e)

    q_3a = get_queue_length(detector_Node3a)
    q_3b = get_queue_length(detector_Node3b)
    q_3c = get_queue_length(detector_Node3c)
    q_3d = get_queue_length(detector_Node3d)
    q_3e = get_queue_length(detector_Node3e)
    q_3f = get_queue_length(detector_Node3f)
    q_3g = get_queue_length(detector_Node3g)

    q_4a = get_queue_length(detector_Node4a)
    q_4b = get_queue_length(detector_Node4b)
    q_4c = get_queue_length(detector_Node4c)
    q_4d = get_queue_length(detector_Node4d)
    q_4e = get_queue_length(detector_Node4e)
    q_4f = get_queue_length(detector_Node4f)
    q_4g = get_queue_length(detector_Node4g)

    q_5a = get_queue_length(detector_Node5a)
    q_5b = get_queue_length(detector_Node5b)
    q_5c = get_queue_length(detector_Node5c)
    q_5d = get_queue_length(detector_Node5d)
    q_5e = get_queue_length(detector_Node5e)
    
    
    # Obtiene la fase actual de la luz de trafico
    current_phase = get_current_phase("1738355191")
    
    return (q_1a,q_1b,q_1c,q_1d,q_1e,q_2a,q_2b,q_2c,q_2d,q_2e,q_3a,q_3b,q_3c,q_3d,q_3e,q_3f,q_3g,q_4a,q_4b,q_4c,q_4d,q_4e,q_4f,q_4g,q_5a,q_5b,q_5c,q_5d,q_5e, current_phase)

def get_queue_length(detector_id): #Obtiene la cantidad de vehiculos en cada detector
    #return traci.lanearea.getJamLengthVehicle(detector_id)
    return traci.lanearea.getLastStepVehicleNumber(detector_id)

def update_vehicle_times(current_time, depart_times, travel_times):
    """
    Actualiza los tiempos de partida para nuevos vehiculos y calcula el tiempo de viaje para vehiculos que llegan
    Imprime el tiempo de viaje para cada vehiculo que llega.
    """
    global current_vehicles
    # Guarda tiempo de partida para cada vehiculo que entra
    for veh_id in traci.vehicle.getIDList():
        if veh_id not in depart_times:
            depart_times[veh_id] = current_time
            current_vehicles += 1

    # Revisa los vehiculos que llegaron en este paso de la simulacion
    arrived_vehicles = traci.simulation.getArrivedIDList()
    for veh_id in arrived_vehicles:
        if veh_id in depart_times:
            # Calcula el tiempo de viaje como la diferencia entre el tiempo actual y el tiempo de aparicion
            travel_times[veh_id] = current_time - depart_times[veh_id]
            print(f"Tiempo de viaje de vehiculo {veh_id}: {travel_times[veh_id]:.2f} segundos")
            current_vehicles -= 1
    
    return current_vehicles
# Paso 8: Ejecutar pasos de simulacion hasta que se llegue al tiempo limite

step_history = []
reward_history = []
queue_history = []
vehicle_history = []
waiting_history = []
emissions_history = []

def get_current_phase(tls_id): 
    return traci.trafficlight.getPhase(tls_id)

# Se colocan semaforos en actuated, debe cambiarse la configuracion de las luces de trafico a actuated
traci.trafficlight.setProgram("1738355191",0)
traci.trafficlight.setProgram("1951912875",0)
traci.trafficlight.setProgram("1056968889",0)
traci.trafficlight.setProgram("1056968883",0)
traci.trafficlight.setProgram("1686155185",0)

while traci.simulation.getMinExpectedNumber() > 0 and  traci.simulation.getTime() < 2000:
    # Aca se realizan las acciones para cada paso de simulacion
    emissions = 0 # Devuelve emisiones de CO2 en mg/s para el ultimo paso, como muestra emisiones por segundo, se resetea
    current_simulation_step = step_count  
    state = get_state()
    traci.simulationStep() # Avanza la simulacion 1

    new_state = get_state()
    for veh_id in traci.vehicle.getIDList():
        emissions += traci.vehicle.getCO2Emission(veh_id) # Calcula las emisiones para cada vehiculo
        if traci.vehicle.getSpeed(veh_id) == 0: # Si la velocidad es cero, suma a la metrica de espera
            waiting_time +=1
    # Ejecuta la funcion de tiempo de semaforo para cada semaforo con sus detectores
    #tiempo_seteado_TFL1 = set_TFL("1738355191","e2_0","e2_1","e2_2","e2_15","e2_16",2,1,tiempo_seteado_TFL1)
    step_count += 1
    if step_count % 100 == 0:
        print(f"Step {step_count}, Current_State: {state}, New_State: {new_state}")
        step_history.append(step_count)
        vehicle_history.append(current_vehicles)
        queue_history.append(sum(new_state[:-1]))  # suma de longitudes de cola
        waiting_history.append(waiting_time)
        emissions_history.append((emissions/1000))  #convierte de mg/s a g/s
        print(emissions/1000)
        print(sum(new_state[:-1]))
    current_time = traci.simulation.getTime()
    current_vehicles = update_vehicle_times(current_time, depart_times, travel_times)


# Paso 9: Cierra la conexion entre SUMO y TRACI
traci.close()

## Se pueden plotear variables
#plt.figure(figsize=(10, 6))
#plt.plot(step_history, queue_history, marker='o', linestyle='-', label="Total Queue Length")
#plt.xlabel("Simulation Step")
#plt.ylabel("Total Queue Length")
#plt.title("Fixed Timing: Queue Length over Steps")
#plt.legend()
#plt.grid(True)
#plt.show()

queue_length = np.array([step_history, queue_history])

# SimpleDataFrame = pd.DataFrame([step_history, queue_history], index=['step_history', 'queue_history']) # filas
Queue_HistoryDF = pd.DataFrame({'step_history': step_history, 'queue_history': queue_history}) # columnas
print(Queue_HistoryDF)

# Se exportan los datos como archivo csv
Queue_HistoryDF.to_csv('C:/sumo_tesis/Queue_HistoryDF3.csv')

vehicle_data = np.array([step_history, vehicle_history])

# SimpleDataFrame = pd.DataFrame([step_history, queue_history], index=['step_history', 'queue_history']) # filas
vehicle_dataDF = pd.DataFrame({'step_history': step_history, 'vehicle_history': vehicle_history}) # columnas
print(vehicle_dataDF)

# Se exportan los datos como archivo csv
vehicle_dataDF.to_csv('C:/sumo_tesis/vehicle_dataDF3.csv')

waiting_data = np.array([step_history, waiting_history])

# SimpleDataFrame = pd.DataFrame([step_history, queue_history], index=['step_history', 'queue_history']) # filas
waiting_dataDF = pd.DataFrame({'step_history': step_history, 'waiting_history': waiting_history}) # columnas
print(waiting_dataDF)

# Se exportan los datos como archivo csv
waiting_dataDF.to_csv('C:/sumo_tesis/waiting_dataDF3.csv')

# SimpleDataFrame = pd.DataFrame([step_history, queue_history], index=['step_history', 'queue_history']) # filas
emissions_dataDF = pd.DataFrame({'step_history': step_history, 'emissions_history': emissions_history}) # columnas
print(emissions_dataDF)

# Se exportan los datos como archivo csv
emissions_dataDF.to_csv('C:/sumo_tesis/emissions_dataDF3.csv')

if travel_times:
    average_travel_time = sum(travel_times.values()) / len(travel_times)
    print(f"Tiempo de viaje promedio para todos los vehiculos: {average_travel_time:.2f} segundos")
else:
    print("No hay datos de tiempo de viaje disponibles.")



        